/*
 * Copyright 2016 the original author or authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file    mds_subscribe_sample.c
 *
 * MDS-API 行情订阅接口的示例程序
 *
 * @version 1.0 2017/2/20
 * @since   2017/2/20
 */


#include    <mds_api/mds_api.h>
#include    <mds_api/parser/mds_protocol_parser.h>
#include    <mds_api/parser/json_parser/mds_json_parser.h>
#include    <sutil/logger/spk_log.h>
#include    <stdio.h>
#include    <sutil/libgo_http.h>
#include    <sutil/time/spk_times.h>


#define L2_TRADE_URL "http://47.105.111.100/OnTrade"
#define L2_ORDER_URL "http://47.105.111.100/OnOrder"
#define L2_TICK_URL "http://47.105.111.100/OnTickL2" 
#define L2_OTHER_DATA_URL "http://47.105.111.100/OtherData"
#define L1_TICK_URL "http://47.105.111.100/OnTickL1"
#define MDS_SUB_SECURITY_STATUS_URL "http://47.105.111.100/OnMdsSubSecurityStatus"
#define MDS_SUB_TRADING_SESSION_STATUS_URL "http://47.105.111.100/onMdsSubTradingSessionStatus"

char sendJsonDataStr[4096];


/**
 * 通过证券代码列表, 重新订阅行情数据 (根据代码前缀区分所属市场)
 *
 * @param   pTcpChannel         会话信息
 * @param   pCodeListString     证券代码列表字符串 (以空格或逗号/分号/竖线分割的字符串)
 * @return  TRUE 成功; FALSE 失败
 */
static BOOL
MdsApiSample_ResubscribeByCodePrefix(MdsApiSessionInfoT *pTcpChannel,
        const char *pCodeListString) {
    /* 上海证券代码前缀 */
    static const char       SSE_CODE_PREFIXES[] = \
            "009, 01, 02, "                 /* 国债 */ \
            "10, 11, 12, 13, 18, 19, "      /* 债券 (企业债、可转债等) */ \
            "20, "                          /* 债券 (回购) */ \
            "5, "                           /* 基金 */ \
            "6, "                           /* A股 */ \
            "#000";                         /* 指数 (@note 与深圳股票代码重合) */

    /* 深圳证券代码前缀 */
    static const char       SZSE_CODE_PREFIXES[] = \
            "00, "                          /* 股票 */ \
            "10, 11, 12, 13, "              /* 债券 */ \
            "15, 16, 17, 18, "              /* 基金 */ \
            "30"                            /* 创业板 */ \
            "39";                           /* 指数 */

    return MdsApi_SubscribeByStringAndPrefixes(pTcpChannel,
            pCodeListString, (char *) NULL,
            SSE_CODE_PREFIXES, SZSE_CODE_PREFIXES,
            MDS_SECURITY_TYPE_STOCK, MDS_SUB_MODE_SET,
            MDS_SUB_DATA_TYPE_L1_SNAPSHOT
                    | MDS_SUB_DATA_TYPE_L2_SNAPSHOT
                    | MDS_SUB_DATA_TYPE_L2_BEST_ORDERS
                    | MDS_SUB_DATA_TYPE_L2_ORDER
                    | MDS_SUB_DATA_TYPE_L2_TRADE);
}


/**
 * 通过证券代码列表, 重新订阅行情数据 (根据代码后缀区分所属市场, 如果没有指定后缀, 则默认为上证股票)
 *
 * @param   pTcpChannel         会话信息
 * @param   pCodeListString     证券代码列表字符串 (以空格或逗号/分号/竖线分割的字符串)
 * @return  TRUE 成功; FALSE 失败
 */
static BOOL
MdsApiSample_ResubscribeByCodePostfix(MdsApiSessionInfoT *pTcpChannel,
        const char *pCodeListString) {
    return MdsApi_SubscribeByString(pTcpChannel,
            pCodeListString, (char *) NULL,
            MDS_EXCH_SSE, MDS_SECURITY_TYPE_STOCK, MDS_SUB_MODE_SET,
            MDS_SUB_DATA_TYPE_L1_SNAPSHOT
                    | MDS_SUB_DATA_TYPE_L2_SNAPSHOT
                    | MDS_SUB_DATA_TYPE_L2_BEST_ORDERS
                    | MDS_SUB_DATA_TYPE_L2_ORDER
                    | MDS_SUB_DATA_TYPE_L2_TRADE);
}


/**
 * 进行消息处理的回调函数
 *
 * @param   pSessionInfo    会话信息
 * @param   pMsgHead        消息头
 * @param   pMsgBody        消息体数据
 * @param   pCallbackParams 外部传入的参数
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static int32
MdsApiSample_HandleMsg(MdsApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, void *pCallbackParams) {

    char                encodeBuf[8192] = {0};
    char                *pStrMsg = (char *) NULL;

    if (pSessionInfo->protocolType == SMSG_PROTO_BINARY) {
        /* 将行情消息转换为JSON格式的文本数据 */
        pStrMsg = (char *) MdsJsonParser_EncodeRsp(
                pMsgHead, (MdsMktRspMsgBodyT *) pMsgBody,
                encodeBuf, sizeof(encodeBuf),
                pSessionInfo->channel.remoteAddr);
    } else {
        pStrMsg = (char *) pMsgBody;
       
    }

    time_t sendDataCurrentTime = STime_GetSysTime();
    time_t GetLastRecvTime = MdsApi_GetLastRecvTime(pSessionInfo);

    if (pMsgHead->msgSize > 0) {
        pStrMsg[pMsgHead->msgSize - 1] = '\0';
        // printf( "{" \
        //         "\"msgType\":%" __SPK_FMT_HH__ "u, " \
        //         "\"mktData\":%s" \
        //         "}\n",
        //         pMsgHead->msgId,
        //         pStrMsg);

        sprintf(sendJsonDataStr,
                "{" \
                "\"msgType\":%" __SPK_FMT_HH__ "u, " \
                "\"sendDCT\":%l, " \
                "\"LastRecvT\":%l, " \
                "\"mktData\":%s" \
                "}\n",
                pMsgHead->msgId,
                sendDataCurrentTime,
                GetLastRecvTime,
                pStrMsg);
    }
    else
    {
        sprintf(sendJsonDataStr,
                "{" \
                "\"msgType\":%" __SPK_FMT_HH__ "u, " \
                "\"sendDCT\":%ld, " \
                "\"LastRecvT\":%ld, " \
                "\"mktData\":{}" \
                "}",
                pMsgHead->msgId,
                sendDataCurrentTime,
                GetLastRecvTime)
                ;
    } 

    char url[150] = "http://47.105.111.100/allData";


    MdsMktRspMsgBodyT   *pRspMsg = (MdsMktRspMsgBodyT *) pMsgBody;
    /*
     * 根据消息类型对行情消息进行处理
     */
    switch (pMsgHead->msgId) {
    case MDS_MSGTYPE_L2_TRADE:
        /* 处理Level2逐笔成交消息 */
        // printf("... 接收到Level2逐笔成交消息 (exchId[%" __SPK_FMT_HH__ "u], instrId[%d])\n",
        //         pRspMsg->trade.exchId,
        //         pRspMsg->trade.instrId);
        strcpy(url,L2_TRADE_URL);

        break;

    case MDS_MSGTYPE_L2_ORDER:
        /* 处理Level2逐笔委托消息 */
        // printf("... 接收到Level2逐笔委托消息 (exchId[%" __SPK_FMT_HH__ "u], instrId[%d])\n",
        //         pRspMsg->order.exchId,
        //         pRspMsg->order.instrId);
        strcpy(url,L2_ORDER_URL);
        break;

    case MDS_MSGTYPE_L2_MARKET_DATA_SNAPSHOT:
    case MDS_MSGTYPE_L2_BEST_ORDERS_SNAPSHOT:
    case MDS_MSGTYPE_L2_MARKET_DATA_INCREMENTAL:
    case MDS_MSGTYPE_L2_BEST_ORDERS_INCREMENTAL:
    case MDS_MSGTYPE_L2_MARKET_OVERVIEW:
    case MDS_MSGTYPE_L2_VIRTUAL_AUCTION_PRICE:
        /* 处理Level2快照行情消息 */
        // printf("... 接收到Level2快照行情消息 (exchId[%" __SPK_FMT_HH__ "u], instrId[%d])\n",
        //         pRspMsg->mktDataSnapshot.head.exchId,
        //         pRspMsg->mktDataSnapshot.head.instrId);
        strcpy(url,L2_TICK_URL);
        break;

    case MDS_MSGTYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH:
    case MDS_MSGTYPE_OPTION_SNAPSHOT_FULL_REFRESH:
    case MDS_MSGTYPE_INDEX_SNAPSHOT_FULL_REFRESH:
        /* 处理Level1快照行情消息 */
        // printf("... 接收到Level1快照行情消息 (exchId[%" __SPK_FMT_HH__ "u], instrId[%d])\n",
        //         pRspMsg->mktDataSnapshot.head.exchId,
        //         pRspMsg->mktDataSnapshot.head.instrId);
        strcpy(url,L1_TICK_URL);
        break;

    case MDS_MSGTYPE_SECURITY_STATUS:
        /* 处理(深圳)证券状态消息 */
        // printf("... 接收到(深圳)证券状态消息 (exchId[%" __SPK_FMT_HH__ "u], instrId[%d])\n",
        //         pRspMsg->securityStatus.exchId,
        //         pRspMsg->securityStatus.instrId);
        strcpy(url,MDS_SUB_SECURITY_STATUS_URL);
        break;

    case MDS_MSGTYPE_TRADING_SESSION_STATUS:
        /* 处理(上证)市场状态消息 */
        // printf("... 接收到(上证)市场状态消息 (exchId[%" __SPK_FMT_HH__ "u], TradingSessionID[%s])\n",
        //         pRspMsg->trdSessionStatus.exchId,
        //         pRspMsg->trdSessionStatus.TradingSessionID);
        strcpy(url,MDS_SUB_TRADING_SESSION_STATUS_URL);
        break;

    case MDS_MSGTYPE_MARKET_DATA_REQUEST:
        /* 处理行情订阅请求的应答消息 */
        if (pMsgHead->status == 0) {
            SLOG_INFO("... 行情订阅请求应答, 行情订阅成功!\n");
        } else {
            SLOG_ERROR("... 行情订阅请求应答, 行情订阅失败! " \
                    "errCode[%02" __SPK_FMT_HH__ "u%02" __SPK_FMT_HH__ "u]\n",
                    pMsgHead->status, pMsgHead->detailStatus);
        }
        break;

    case MDS_MSGTYPE_TEST_REQUEST:
        /* 处理测试请求的应答消息 */
        SLOG_INFO("... 接收到测试请求的应答消息 (origSendTime[%s], respTime[%s])\n",
                pRspMsg->testRequestRsp.origSendTime,
                pRspMsg->testRequestRsp.respTime);
        break;

    case MDS_MSGTYPE_HEARTBEAT:
        /* 忽略心跳消息 */
        break;

    default:
        SLOG_ERROR("无效的消息类型, 忽略之! msgId[0x%02X], server[%s:%d]",
                pMsgHead->msgId, pSessionInfo->channel.remoteAddr,
                pSessionInfo->channel.remotePort);
        return EFTYPE;
    }

    int length = strlen(sendJsonDataStr);
    int ulength = strlen(url);
    GoString gMsg = {sendJsonDataStr,length};
    GoString gUrl = {url,ulength};
    GoInt httpRes = -1;
    httpRes =  httpGet(gUrl,gMsg);

    if(httpRes != 1)
    {
        SLOG_ERROR("...httpGet,ERROR,mds_subscribe,ulength is: %s,%d,%d,%s",url,length,httpRes,sendJsonDataStr);
    }

    return 0;
}


int
main(int argc, char *argv[]) {
    /* 配置文件 */
    static const char   THE_CONFIG_FILE_NAME[] = "mds_subscribe.conf";
    /* 尝试等待行情消息到达的超时时间 (毫秒) */
    static const int32  THE_TIMEOUT_MS = 1000;

    MdsApiClientEnvT    cliEnv = {NULLOBJ_MDSAPI_CLIENT_ENV};
    int32               ret = 0;

    /* 初始化客户端环境 (配置文件参见: mds_client_sample.conf) */
    if (! MdsApi_InitAllByConvention(&cliEnv, THE_CONFIG_FILE_NAME)) {
        return -1;
    }

    if (1) {
        /* 根据证券代码列表重新订阅行情 (根据代码前缀区分所属市场) */
        if (! MdsApiSample_ResubscribeByCodePrefix(&cliEnv.tcpChannel,
                "601881.SH")) {
            goto ON_ERROR;
        }
    } else {
        /* 根据证券代码列表重新订阅行情 (根据代码后缀区分所属市场) */
        if (! MdsApiSample_ResubscribeByCodePostfix(&cliEnv.tcpChannel,
                "600000.SH, 600001.SH, 000001.SZ, 0000002.SZ")) {
            goto ON_ERROR;
        }
    }

    while (1) {
        /* 等待行情消息到达, 并通过回调函数对消息进行处理 */
        ret = MdsApi_WaitOnMsg(&cliEnv.tcpChannel, THE_TIMEOUT_MS,
                MdsApiSample_HandleMsg, NULL);
        if (unlikely(ret < 0)) {
            if (likely(SPK_IS_NEG_ETIMEDOUT(ret))) {
                /* 执行超时检查 (检查会话是否已超时) */
                continue;
            }

            if (SPK_IS_NEG_EPIPE(ret)) {
                /* 连接已断开 */
            }
            goto ON_ERROR;
        }
    }

    MdsApi_LogoutAll(&cliEnv, TRUE);
    return 0;

ON_ERROR:
    MdsApi_DestoryAll(&cliEnv);
    return -1;
}
