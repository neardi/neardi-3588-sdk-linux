/*
 *
 *  Seekwave Bluetooth driver
 *
 *  Copyright (C) 2023  Seekwave Tech Ltd.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/platform_device.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
//#include <linux/platform_data/skw_platform_data.h>
#include <skw_platform_data.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/firmware.h>
#include <linux/notifier.h>
#include <linux/delay.h>

#include "skw_btsnoop.h"
#include "skw_log.h"
#include "skw_common.h"

#define VERSION "0.1"




#define NV_FILE_RD_BLOCK_SIZE    252

#define HCI_CMD_READ_LOCAL_VERSION_INFO 0x1001

#define HCI_CMD_SKW_BT_NVDS      0xFC80
#define HCI_CMD_WRITE_BD_ADDR    0xFC82
#define HCI_CMD_WRITE_BT_STATE   0xFE80

#define HCI_COMMAND_COMPLETE_EVENT      0x0E
#define HCI_EVT_HARDWARE_ERROR          0x10

enum
{
    BT_STATE_DEFAULT = 0x00,
    BT_STATE_CLOSE,
    BT_STATE_REMOVE
};

int skwbt_log_disable = 0;
int is_init_mode = 0;
uint16_t chip_version = 0;
wait_queue_head_t nv_wait_queue;
wait_queue_head_t recovery_wait_queue;
wait_queue_head_t close_wait_queue;

atomic_t evt_recv;
atomic_t cmd_reject;
atomic_t atomic_close_sync;//make sure running close func before remove func

static int btseekwave_send_frame(struct hci_dev *hdev, struct sk_buff *skb);

extern int skw_start_bt_service(void);
extern int skw_stop_bt_service(void);
struct btseekwave_data
{
    struct hci_dev   *hdev;
    struct sv6160_platform_data *pdata;

    struct work_struct work;

    struct notifier_block plt_notifier;

    struct sk_buff_head cmd_txq;
    struct sk_buff_head data_txq;
    struct sk_buff_head audio_txq;
};


void btseekwave_hci_hardware_error(struct hci_dev *hdev)
{
    struct sk_buff *skb = NULL;
    int len = 3;
    uint8_t hw_err_pkt[4] = {HCI_EVENT_PKT, HCI_EVT_HARDWARE_ERROR, 0x01, 0x00};

    skb = alloc_skb(len, GFP_ATOMIC);
    if (!skb)
    {
        pr_err("%s: failed to allocate mem", __func__);
        return;
    }
    memcpy(skb_put(skb, len), hw_err_pkt + 1, len);
    bt_cb(skb)->pkt_type = HCI_EVENT_PKT;
    hci_recv_frame(hdev, skb);
}


static int btseekwave_tx_packet(int portno, struct btseekwave_data *data, struct sk_buff *skb)
{
    int err = 0;
    u32 *d;

    d = (u32 *)skb->data;

    //pr_info("%s enter size %d: 0x%x 0x%x\n", __func__, skb->len, d[0], d[1]);

    if(data->pdata && data->pdata->hw_sdma_tx)
    {
        err = data->pdata->hw_sdma_tx(portno, skb->data, skb->len);
    }
    if (err < 0)
    {
        return err;
    }

    data->hdev->stat.byte_tx += skb->len;

    //pr_info("%s, pkt:%d, users:%d \n", __func__, bt_cb((skb))->pkt_type, skb->users.refs.counter);
    kfree_skb(skb);

    return 0;
}

static void btseekwave_work(struct work_struct *work)
{
    struct btseekwave_data *data = container_of(work, struct btseekwave_data, work);
    struct sk_buff *skb;
    int err = 0;

    //pr_info("%s %s", __func__, data->hdev->name);

    if(atomic_read(&cmd_reject))
    {
        return ;
    }

    while ((skb = skb_dequeue(&data->cmd_txq)))
    {
        err = btseekwave_tx_packet(data->pdata->cmd_port, data, skb);
        if (err < 0)
        {
            data->hdev->stat.err_tx++;
            skb_queue_head(&data->cmd_txq, skb);
            pr_err("btseekwave_tx_packet command failed len: %d\n", err);
            break;
        }
    }

    while (err >= 0 && (skb = skb_dequeue(&data->data_txq)))
    {
        err = btseekwave_tx_packet(data->pdata->data_port, data, skb);
        if (err < 0)
        {
            data->hdev->stat.err_tx++;
            skb_queue_head(&data->data_txq, skb);
            pr_err("btseekwave_tx_packet data failed len: %d\n", err);
            break;
        }
    }
    while (err >= 0 && (skb = skb_dequeue(&data->audio_txq)))
    {
        err = btseekwave_tx_packet(data->pdata->audio_port, data, skb);
        if (err < 0)
        {
            data->hdev->stat.err_tx++;
            skb_queue_head(&data->audio_txq, skb);
            pr_err("btseekwave_tx_packet audio failed len: %d\n", err);
            break;
        }
    }
//  pr_info("btseekwave_work done\n");
}


static int btseekwave_rx_packet(struct btseekwave_data *data, u8 pkt_type, void *buf, int c_len)
{
    struct sk_buff *skb;
    //pr_info("rx hci pkt len = %d, pkt_type:%d, data = 0x%x\n", skb->len, pkt_type, d[0]);

    skb = bt_skb_alloc(c_len, GFP_ATOMIC);
    if (!skb)
    {
        pr_err("skwbt alloc skb failed, len: %d\n", c_len);
        return 0;
    }
    bt_cb((skb))->expect = 0;
    skb->dev = (void *) data->hdev;
    bt_cb(skb)->pkt_type = pkt_type;
    memcpy(skb_put(skb, c_len), buf, c_len);

    hci_recv_frame(data->hdev, skb);

    return 0;
}

int btseekwave_rx_complete(int portno, struct scatterlist *priv, int size, void *buf)
{
    int ret = 0;
    struct btseekwave_data *data = (struct btseekwave_data *)priv;
    u8 pkt_type = 0;

    //pr_info("btseekwave_rx_complete size=%d\n", size);
    if(size == 0)
    {
        return 0;
    }
    else if(size < 0)//CP assert/exception
    {
        pr_err("cp exception\n");
        return 0;
    }
    pkt_type = *((u8 *)buf);
    if(HCI_EVENT_SKWLOG == pkt_type)
    {
#if BT_CP_LOG_EN
        skwlog_write(buf, size);
#endif
        return 0;
    }


    if((HCI_EVENT_PKT == pkt_type) || (HCI_ACLDATA_PKT == pkt_type) || (HCI_SCODATA_PKT == pkt_type))
    {
#if BT_HCI_LOG_EN
        skw_btsnoop_capture(buf, 1);
#endif

        if(is_init_mode)//command complete event
        {
            hci_cmd_cmpl_evt_st *hci_evt = (hci_cmd_cmpl_evt_st *)buf;
            if((HCI_EVENT_PKT == pkt_type) && (HCI_COMMAND_COMPLETE_EVENT == hci_evt->evt_op) && (HCI_CMD_READ_LOCAL_VERSION_INFO == hci_evt->cmd_op))
            {
                struct hci_rp_read_local_version *ver;
                ver = (struct hci_rp_read_local_version *)(buf + 6);
                chip_version = le16_to_cpu(ver->hci_rev);
                BT_ERR("%s, chip version:0x%X", __func__, chip_version);
            }

            atomic_inc(&evt_recv);
            wake_up(&nv_wait_queue);
            pr_info("init cmd response: 0x%x \n", *((u32 *)(buf + 3)));
            return 0;
        }


        ret = btseekwave_rx_packet(data, pkt_type, buf + 1, size - 1);
    }
    else
    {
        pr_err("err hci packet: %x, len:%d\n", pkt_type, size);
    }

    return ret;
}

struct sk_buff *btseekwave_prepare_cmd(struct hci_dev *hdev, u16 opcode, u32 plen,
                                       const void *param)
{
    int len = HCI_COMMAND_HDR_SIZE + plen;
    struct hci_command_hdr *hdr;
    struct sk_buff *skb;

    skb = bt_skb_alloc(len, GFP_ATOMIC);
    if (!skb)
    {
        return NULL;
    }

    hdr = (struct hci_command_hdr *) skb_put(skb, HCI_COMMAND_HDR_SIZE);
    hdr->opcode = cpu_to_le16(opcode);
    hdr->plen   = plen;

    if (plen)
    {
        memcpy(skb_put(skb, plen), param, plen);
    }


    bt_cb(skb)->pkt_type = HCI_COMMAND_PKT;

    return skb;
}

void btseekwave_write_bd_addr(struct hci_dev *hdev)
{
    u8 cmd_pld[32] = {0x12, 0x34, 0xAB, 0xED, 0x6A, 0x56};//random addr
    struct sk_buff *skb;
    if(!skw_get_bd_addr(cmd_pld))//bd addr is invalid
    {
        return ;
    }
    skb = btseekwave_prepare_cmd(hdev, HCI_CMD_WRITE_BD_ADDR, BD_ADDR_LEN, cmd_pld);
    if(skb)
    {
        btseekwave_send_frame(hdev, skb);
        atomic_set(&evt_recv, 0);
        wait_event_interruptible_timeout(nv_wait_queue,
                                         (atomic_read(&evt_recv)),
                                         msecs_to_jiffies(2000));

    }
    else
    {
        pr_info("%s no memory for nv command", __func__);
    }
}


/*
0: success
other:fail
*/
int btseekwave_send_hci_command(struct hci_dev *hdev, u16 opcode, int len, char *cmd_pld)
{
    struct sk_buff *skb;
    int ret = 0;

    skb = btseekwave_prepare_cmd(hdev, opcode, len, cmd_pld);
    if(!skb)
    {
        pr_info("%s no memory for nv command", __func__);
        return -1;
    }
    btseekwave_send_frame(hdev, skb);

    //waiting controller response
    atomic_set(&evt_recv, 0);
    ret = wait_event_interruptible_timeout(nv_wait_queue,
                                           (atomic_read(&evt_recv)),
                                           msecs_to_jiffies(1000));
    if(ret > 0)
    {
        return 0;
    }
    pr_info("%s cp response timeout", __func__);
    return -1;
}

int btseekwave_download_nv(struct hci_dev *hdev)
{
    int page_offset = 0, ret = 0, len = 0;
    u8 *cmd_pld = NULL;
    const struct firmware *fw;
    int err, count = 0;

    pr_info("%s", __func__);

    is_init_mode = 1;
    chip_version = SKW_CHIPID_6160;

    ret = btseekwave_send_hci_command(hdev, HCI_CMD_READ_LOCAL_VERSION_INFO, 0, NULL);
    if(ret < 0)
    {
        BT_ERR("%s, read local version err", __func__);
        return -1;
    }

    if(SKW_CHIPID_6316 == chip_version)
    {
        err = request_firmware(&fw, NV_FILE_NAME_6316, &hdev->dev);
    }
    else
    {
        err = request_firmware(&fw, NV_FILE_NAME, &hdev->dev);
    }
    if (err < 0)
    {
        pr_err("%s file load fail", NV_FILE_NAME);
        return err;
    }
    cmd_pld = (u8 *)kzalloc(512, GFP_KERNEL);
    if(cmd_pld == NULL)
    {
        pr_err("%s malloc fail", __func__);
        release_firmware(fw);
        return -1;
    }

    if(SKW_CHIPID_6316 == chip_version)
    {
        int total_len = 0;
        int nv_pkt_len = 0;
        count = 4;//skip header
        while(count < fw->size)
        {
            nv_pkt_len = fw->data[count + 2] + 3;
            if((nv_pkt_len + total_len) >= NV_FILE_RD_BLOCK_SIZE)
            {
                cmd_pld[0] = (char)page_offset;
                cmd_pld[1] = (char)total_len;//para len
                ret = btseekwave_send_hci_command(hdev, HCI_CMD_SKW_BT_NVDS, total_len + 2, cmd_pld);
                if(ret < 0)
                {
                    return -1;
                }
                page_offset ++;
                total_len = 0;
                continue;
            }
            memcpy(cmd_pld + 2 + total_len, fw->data + count, nv_pkt_len);
            count += nv_pkt_len;
            total_len += nv_pkt_len;
        }
        if(total_len > 0)
        {
            cmd_pld[0] = (char)page_offset;
            cmd_pld[1] = (char)total_len;//para len
            ret = btseekwave_send_hci_command(hdev, HCI_CMD_SKW_BT_NVDS, total_len + 2, cmd_pld);
            if(ret < 0)
            {
                return -1;
            }
        }
    }
    else
    {
        skwbt_log_disable = fw->data[0x131];

        while(count < fw->size)
        {
            len = NV_FILE_RD_BLOCK_SIZE;
            if((fw->size - count) < NV_FILE_RD_BLOCK_SIZE)
            {
                len = fw->size - count;
            }
            cmd_pld[0] = (char)page_offset;
            cmd_pld[1] = (char)len;//para len
            memcpy(cmd_pld + 2, fw->data + count, len);
            count += len;

            ret = btseekwave_send_hci_command(hdev, HCI_CMD_SKW_BT_NVDS, len + 2, cmd_pld);
            if(ret < 0)
            {
                pr_info("%s cp response timeout", __func__);
                break;
            }
            page_offset ++;
        }
    }

    btseekwave_write_bd_addr(hdev);

    kfree(cmd_pld);
    release_firmware(fw);
    is_init_mode = 0;

    return 0;
}


static int btseekwave_open(struct hci_dev *hdev)
{
    struct btseekwave_data *data = hci_get_drvdata(hdev);
    int err = -1;

    pr_info("%s enter...\n", __func__);

    if(atomic_read(&cmd_reject))
    {
        int ret = wait_event_interruptible_timeout(recovery_wait_queue,
                  (!atomic_read(&cmd_reject)),
                  msecs_to_jiffies(2000));
        if(!ret)
        {
            pr_info("%s timeout", __func__);
            return ret;
        }
    }

    if(data && data->pdata && data->pdata->open_port)
    {
        err = data->pdata->open_port(data->pdata->cmd_port, btseekwave_rx_complete,  data);

        pr_info("%s mode data_port:%d, audio_port:%d\n", __func__, data->pdata->data_port, data->pdata->audio_port);

        if((!err) && (data->pdata->data_port != 0))
        {
            err = data->pdata->open_port(data->pdata->data_port, btseekwave_rx_complete, data);
        }
        if((!err) && (data->pdata->audio_port != 0))
        {
            err = data->pdata->open_port(data->pdata->audio_port, btseekwave_rx_complete, data);
        }
#if INCLUDE_NEW_VERSION
        if(data->pdata->service_start)
        {
            err = data->pdata->service_start();
            if(err != 0)
            {
                pr_err("func %s service_start err:%d", __func__, err);
                return err;
            }
        }
        else
        {
            pr_err("func %s service_start not exist", __func__);
        }
#else
        skw_start_bt_service();
#endif
        err = btseekwave_download_nv(hdev);
    }
    atomic_set(&atomic_close_sync, 0);
    return err;
}

void btseekwave_write_bt_state(struct hci_dev *hdev)
{
    //char buffer[10] = {0x01, 0x80, 0xFE, 0x01, 0x00};
    u8 cmd_pld[5] = {0x00};
    struct sk_buff *skb = btseekwave_prepare_cmd(hdev, HCI_CMD_WRITE_BT_STATE, 1, cmd_pld);
    if(skb)
    {
        btseekwave_send_frame(hdev, skb);
        msleep(15);
    }
}


static int btseekwave_close(struct hci_dev *hdev)
{
    struct btseekwave_data *data = hci_get_drvdata(hdev);
    int state = 0;

    pr_info("%s enter...\n", __func__);

    if(data && (data->pdata->data_port == 0))
    {
        btseekwave_write_bt_state(hdev);
    }

    if(atomic_read(&cmd_reject))
    {
        int ret = wait_event_interruptible_timeout(recovery_wait_queue,
                  (!atomic_read(&cmd_reject)),
                  msecs_to_jiffies(2000));
        if(!ret)
        {
            pr_info("%s timeout", __func__);
            return ret;
        }
    }
    if(data && data->pdata)
    {
        if(data->pdata->modem_unregister_notify)
        {
            data->pdata->modem_unregister_notify(&data->plt_notifier);
        }
        if(data->pdata->close_port)
        {
            data->pdata->close_port(data->pdata->cmd_port);
            if(data->pdata->data_port != 0)
            {
                data->pdata->close_port(data->pdata->data_port);
            }
            if(data->pdata->audio_port != 0)
            {
                data->pdata->close_port(data->pdata->audio_port);
            }
        }
#if INCLUDE_NEW_VERSION
        if(data->pdata->service_stop)
        {
            data->pdata->service_stop();
        }
        else
        {
            pr_err("func %s service_stop not exist", __func__);
        }
#else
        skw_stop_bt_service();
#endif
    }

    state = atomic_read(&atomic_close_sync);
    pr_info("func %s, atomic_read:%d", __func__, state);

    if(state == BT_STATE_DEFAULT)
    {
        atomic_set(&atomic_close_sync, BT_STATE_CLOSE);
    }
    else
    {
        atomic_set(&atomic_close_sync, BT_STATE_CLOSE);
        wake_up(&close_wait_queue);
    }
    return 0;
}

static int btseekwave_flush(struct hci_dev *hdev)
{
    struct btseekwave_data *data = hci_get_drvdata(hdev);

    pr_info("%s", hdev->name);

    if (work_pending(&data->work))
    {
        cancel_work_sync(&data->work);
    }

    skb_queue_purge(&data->cmd_txq);
    skb_queue_purge(&data->data_txq);
    skb_queue_purge(&data->audio_txq);

    return 0;
}

static int btseekwave_send_frame(struct hci_dev *hdev, struct sk_buff *skb)
{
    struct btseekwave_data *data = hci_get_drvdata(hdev);
    u8 pkt_type = bt_cb(skb)->pkt_type;
    u8 *d = skb_push(skb, 1);
    *d = pkt_type;

    if(data->pdata == NULL)
    {
        pr_err("%s pointer is null", __func__);
        return -EILSEQ;
    }
    if((pkt_type == HCI_COMMAND_PKT) || ((pkt_type == HCI_ACLDATA_PKT) && (data->pdata->data_port == 0))
            || ((pkt_type == HCI_SCODATA_PKT) && (data->pdata->audio_port == 0)))
    {
        hdev->stat.cmd_tx++;
        skb_queue_tail(&data->cmd_txq, skb);
    }
    else if(pkt_type == HCI_ACLDATA_PKT)
    {
        hdev->stat.acl_tx++;
        skb_queue_tail(&data->data_txq, skb);
    }
    else if(pkt_type == HCI_SCODATA_PKT)
    {
        skb_queue_tail(&data->audio_txq, skb);
        hdev->stat.sco_tx++;
    }
    else
    {
        return -EILSEQ;
    }

#if BT_HCI_LOG_EN
    skw_btsnoop_capture(skb->data, 0);
#endif
    schedule_work(&data->work);

    return 0;
}


static int btseekwave_setup(struct hci_dev *hdev)
{
    pr_info("%s", __func__);
    return 0;
}


/*
must be in DEVICE_ASSERT_EVENT to DEVICE_DUMPDONE_EVENT closing USB
*/
int btseekwave_plt_event_notifier(struct notifier_block *nb, unsigned long action, void *data)
{
    pr_info("%s, action:%d", __func__, (int)action);
#if 0
    switch(action)
    {
        case DEVICE_ASSERT_EVENT:
        {
            //struct btseekwave_data *data = container_of(nb, struct btseekwave_data, plt_notifier);

            //make surce host data cann't send to plt driver before close usb
            atomic_set(&cmd_reject, 1);
            skw_stop_bt_service();
        }
        break;
        case DEVICE_BSPREADY_EVENT:
        {
            atomic_set(&cmd_reject, 0);
            wake_up(&recovery_wait_queue);
        }
        break;
        case DEVICE_DUMPDONE_EVENT:
        {
            struct btseekwave_data *data = container_of(nb, struct btseekwave_data, plt_notifier);
            btseekwave_hci_hardware_error(data->hdev);//repo to host
        }
        break;
        case DEVICE_BLOCKED_EVENT:
        {

        }
        break;
        default:
        {

        }
        break;

    }
#endif
    return NOTIFY_OK;
}


static int btseekwave_probe(struct platform_device *pdev)
{
    struct btseekwave_data *data;
    struct device *dev = &pdev->dev;
    struct sv6160_platform_data *pdata = dev->platform_data;
    struct hci_dev *hdev;
    int err;
    if(pdata == NULL)
    {
        pr_info("%s pdata is null", __func__);
        return -ENOMEM;
    }

    pr_info("%s pdev name %s\n", __func__, pdata->port_name);

    data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
    if (!data)
    {
        return -ENOMEM;
    }
    data->plt_notifier.notifier_call = btseekwave_plt_event_notifier;
    pdata->modem_register_notify(&data->plt_notifier);

    data->pdata = pdata;

    INIT_WORK(&data->work, btseekwave_work);

    skb_queue_head_init(&data->cmd_txq);
    skb_queue_head_init(&data->data_txq);
    skb_queue_head_init(&data->audio_txq);

    hdev = hci_alloc_dev();
    if (!hdev)
    {
        return -ENOMEM;
    }

    hdev->bus = HCI_SDIO;
    hci_set_drvdata(hdev, data);

    data->hdev = hdev;

    SET_HCIDEV_DEV(hdev, dev);

    hdev->open     = btseekwave_open;
    hdev->close    = btseekwave_close;
    hdev->flush    = btseekwave_flush;
    hdev->send     = btseekwave_send_frame;
    hdev->setup    = btseekwave_setup;

    atomic_set(&hdev->promisc, 0);


    err = hci_register_dev(hdev);
    if (err < 0)
    {
        hci_free_dev(hdev);
        return err;
    }

    platform_set_drvdata(pdev, data);

    skw_bd_addr_gen_init();
    atomic_set(&cmd_reject, 0);
    atomic_set(&atomic_close_sync, BT_STATE_DEFAULT);

    return 0;
}

static int btseekwave_remove(struct platform_device *pdev)
{
    int state = atomic_read(&atomic_close_sync);

    pr_info("func %s, atomic_read:%d", __func__, state);

    if(BT_STATE_DEFAULT == state)
    {
        atomic_set(&atomic_close_sync, BT_STATE_REMOVE);
        wait_event_interruptible_timeout(close_wait_queue,
                                         (BT_STATE_CLOSE == atomic_read(&atomic_close_sync)),
                                         msecs_to_jiffies(500));
    }

    atomic_set(&atomic_close_sync, BT_STATE_DEFAULT);
    if(pdev)
    {
        struct btseekwave_data *data = platform_get_drvdata(pdev);
        struct hci_dev *hdev;

        if (!data)
        {
            return 0;
        }
        hdev = data->hdev;

        btseekwave_flush(hdev);

        platform_set_drvdata(pdev, NULL);

        hci_unregister_dev(hdev);

        hci_free_dev(hdev);
    }
    pr_info("func %s end", __func__);
    return 0;
}

static struct platform_driver  btseekwave_driver =
{
    .driver = {
        .name   = (char *)"btseekwave",
        .bus    = &platform_bus_type,
        .pm     = NULL,
    },
    .probe      = btseekwave_probe,
    .remove     = btseekwave_remove,
};

int  btseekwave_init(void)
{
    pr_info("Seekwave Bluetooth driver ver %s\n", VERSION);
    init_waitqueue_head(&nv_wait_queue);
    init_waitqueue_head(&recovery_wait_queue);
    init_waitqueue_head(&close_wait_queue);
    atomic_set(&evt_recv, 0);

#if BT_HCI_LOG_EN
    skw_btsnoop_init();
#endif
#if BT_CP_LOG_EN
    skwlog_init();
#endif

    return platform_driver_register(&btseekwave_driver);
}

void  btseekwave_exit(void)
{
#if BT_HCI_LOG_EN
    skw_btsnoop_close();
#endif
#if BT_CP_LOG_EN
    skwlog_close();
#endif

    platform_driver_unregister(&btseekwave_driver);
}

module_init(btseekwave_init);
module_exit(btseekwave_exit);

MODULE_DESCRIPTION("Seekwave Bluetooth driver ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
