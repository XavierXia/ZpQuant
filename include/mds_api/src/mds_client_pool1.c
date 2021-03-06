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
 * @file    mds_client.c
 *
 * MDS API接口库
 *
 * @version 1.0 2016/7/26
 * @since   2016/7/26
 */


#include    <mds_api/mds_api.h>
#include    <mds_api/parser/mds_protocol_parser.h>
#include    <mds_api/parser/json_parser/mds_json_parser.h>
#include    <sutil/time/spk_times.h>
#include    <sutil/logger/spk_log.h>
#include    <stdio.h>
#include    <sutil/libgo_http.h>
#include    <sutil/thread_pool.h>
#include    <sutil/msg_log.h>

#define LOG_INFO
/*
#define L2_TRADE_URL "http://127.0.0.1/OnTrade"
#define L2_ORDER_URL "http://127.0.0.1/OnOrder"
#define L2_TICK_URL "http://127.0.0.1/OnTickL2" 
#define L2_OTHER_DATA_URL "http://127.0.0.1/OtherData"
#define L1_TICK_URL "http://127.0.0.1/OnTickL1" 
*/

/*
#define L2_TRADE_URL "http://47.105.111.100/OnTrade"
#define L2_ORDER_URL "http://47.105.111.100/OnOrder"
#define L2_TICK_URL "http://47.105.111.100/OnTickL2" 
#define L2_OTHER_DATA_URL "http://47.105.111.100/OtherData"
#define L1_TICK_URL "http://47.105.111.100/OnTickL1" 
*/

#define KR_HQ_ALLDATA_URL "http://47.105.111.100/OnKrHQAllData"


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char sendJsonDataStr[4096];

/**
 * 将行情消息转换为JSON格式的文本, 并打印到指定的输出文件
 *
 * @param   pSessionInfo    会话信息
 * @param   pMsgHead        消息头
 * @param   pMsgBody        消息体数据
 * @param   pOutputFp       输出文件的文件句柄
 * @return  等于0，成功；小于0，失败（错误号）
 */
static inline int32
_MdsApiSample_PrintMsg(MdsApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, FILE *pOutputFp) {
    char                encodeBuf[8192] = {0};
    char                *pStrMsg = (char *) NULL;

    if (pSessionInfo->protocolType == SMSG_PROTO_BINARY) {
        /* 将行情消息转换为JSON格式的文本数据 */
        pStrMsg = (char *) MdsJsonParser_EncodeRsp(
                pMsgHead, (MdsMktRspMsgBodyT *) pMsgBody,
                encodeBuf, sizeof(encodeBuf),
                pSessionInfo->channel.remoteAddr);
        if (unlikely(! pStrMsg)) {
            SLOG_ERROR("编码接收到的行情消息失败! "
                    "msgFlag: %" __SPK_FMT_HH__ "u, " \
                    "msgType: %" __SPK_FMT_HH__ "u, msgSize: %d",
                    pMsgHead->msgFlag, pMsgHead->msgId, pMsgHead->msgSize);
            return NEG(EBADMSG);
        }
    } else {
        pStrMsg = (char *) pMsgBody;
    }

    time_t sendDataCurrentTime = STime_GetMillisecondsTime();
    time_t GetLastRecvTime = MdsApi_GetLastRecvTime(pSessionInfo);

    if (pMsgHead->msgSize > 0) {
        pStrMsg[pMsgHead->msgSize - 1] = '\0';
        // fprintf(pOutputFp,
        //         "{" \
        //         "\"msgType\":%" __SPK_FMT_HH__ "u, " \
        //         "\"mktData\":%s" \
        //         "}\n",
        //         pMsgHead->msgId,
        //         pStrMsg);
        
        
        sprintf(sendJsonDataStr,
                "{" \
                "\"msgType\":%" __SPK_FMT_HH__ "u, " \
                "\"sendDCT\":%ld, " \
                "\"LastRecvT\":%ld, " \
                "\"mktData\":%s" \
                "}",
                pMsgHead->msgId,
                sendDataCurrentTime,
                GetLastRecvTime,
                pStrMsg);
         
        //sprintf(sendJsonDataStr,
        //        "{" \
        //        "\"msgType\":%" __SPK_FMT_HH__ "u, " \
        //        "\"mktData\":%s" \
        //        "}",
        //        1,
        //        "aaa");


    
    
    } else {
        // fprintf(pOutputFp,
        //         "{" \
        //         "\"msgType\":%" __SPK_FMT_HH__ "u, " \
        //         "\"mktData\":{}" \
        //         "}\n",
        //         pMsgHead->msgId);

        sprintf(sendJsonDataStr,
                "{" \
                "\"msgType\":%" __SPK_FMT_HH__ "u, " \
                "\"sendDCT\":%ld, " \
                "\"LastRecvT\":%ld, " \
                "\"mktData\":{}" \
                "}",
                pMsgHead->msgId,
                sendDataCurrentTime,
                GetLastRecvTime);
    }

    char url[60] = "http://47.105.111.100/OnKrHQAllData";

    int length = strlen(sendJsonDataStr);
    int ulength = strlen(url);
    GoString gMsg = {sendJsonDataStr,length};
    GoString gUrl = {url,ulength};
    GoInt httpRes = -1;
    httpRes =  httpGet(gUrl,gMsg);

    if(httpRes != 1)
    {
        SLOG_ERROR("...httpGet,ERROR,mds_client,ulength is: %s,%d,%d,%s",url,length,httpRes,sendJsonDataStr);
    }

    return 0;
}


/**
 * 证券行情全幅消息处理
 *
 * @param   pSessionInfo    会话信息
 * @param   pMsgHead        消息头
 * @param   pMsgBody        消息体数据
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static inline int32
_MdsApiSample_OnSnapshotFullRefresh(MdsApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, MdsMktDataSnapshotT *pRspBody) {
    //TODO 证券行情全幅消息处理

    /* 打印行情消息到控制台 */
    return _MdsApiSample_PrintMsg(pSessionInfo, pMsgHead, pRspBody, stdout);
}


/**
 * Level2逐笔成交消息处理
 *
 * @param   pSessionInfo    会话信息
 * @param   pMsgHead        消息头
 * @param   pMsgBody        消息体数据
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static inline int32
_MdsApiSample_OnL2Trade(MdsApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, MdsL2TradeT *pRspBody) {
    //TODO Level2逐笔成交消息处理

    /* 打印行情消息到控制台 */
    return _MdsApiSample_PrintMsg(pSessionInfo, pMsgHead, pRspBody, stdout);
}


/**
 * Level2逐笔委托消息处理
 *
 * @param   pSessionInfo    会话信息
 * @param   pMsgHead        消息头
 * @param   pMsgBody        消息体数据
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static inline int32
_MdsApiSample_OnL2Order(MdsApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, MdsL2OrderT *pRspBody) {
    //TODO Level2逐笔委托消息处理

    /* 打印行情消息到控制台 */
    return _MdsApiSample_PrintMsg(pSessionInfo, pMsgHead, pRspBody, stdout);
}


/**
 * (上证)市场状态消息处理
 *
 * @param   pSessionInfo    会话信息
 * @param   pMsgHead        消息头
 * @param   pMsgBody        消息体数据
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static inline int32
_MdsApiSample_OnTradingSessionStatus(MdsApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, MdsTradingSessionStatusMsgT *pRspBody) {
    //TODO (上证)市场状态消息处理

    /* 打印行情消息到控制台 */
    return _MdsApiSample_PrintMsg(pSessionInfo, pMsgHead, pRspBody, stdout);
}


/**
 * (深圳)证券状态消息处理
 *
 * @param   pSessionInfo    会话信息
 * @param   pMsgHead        消息头
 * @param   pMsgBody        消息体数据
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static inline int32
_MdsApiSample_OnSecurityStatus(MdsApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, MdsSecurityStatusMsgT *pRspBody) {
    //TODO (深圳)证券状态消息处理

    /* 打印行情消息到控制台 */
    return _MdsApiSample_PrintMsg(pSessionInfo, pMsgHead, pRspBody, stdout);
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
static inline int32
_MdsApiSample_HandleMsg(MdsApiSessionInfoT *pSessionInfo,
        SMsgHeadT *pMsgHead, void *pMsgBody, void *pCallbackParams) {
    MdsMktRspMsgBodyT   *pRspMsg = (MdsMktRspMsgBodyT *) pMsgBody;

    /*
     * 根据消息类型对行情消息进行处理
     */
    switch (pMsgHead->msgId) {
    case MDS_MSGTYPE_L2_TRADE:
        /* 处理Level2逐笔成交消息 */
        return _MdsApiSample_OnL2Trade(
                pSessionInfo, pMsgHead, &pRspMsg->trade);

    case MDS_MSGTYPE_L2_ORDER:
        /* 处理Level2逐笔委托消息 */
        return _MdsApiSample_OnL2Order(
                pSessionInfo, pMsgHead, &pRspMsg->order);

    case MDS_MSGTYPE_L2_MARKET_DATA_SNAPSHOT:
    case MDS_MSGTYPE_L2_BEST_ORDERS_SNAPSHOT:
    case MDS_MSGTYPE_L2_MARKET_DATA_INCREMENTAL:
    case MDS_MSGTYPE_L2_BEST_ORDERS_INCREMENTAL:
    case MDS_MSGTYPE_L2_MARKET_OVERVIEW:
    case MDS_MSGTYPE_L2_VIRTUAL_AUCTION_PRICE:
    case MDS_MSGTYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH:
    case MDS_MSGTYPE_OPTION_SNAPSHOT_FULL_REFRESH:
    case MDS_MSGTYPE_INDEX_SNAPSHOT_FULL_REFRESH:
        /* 处理证券行情全幅消息 */
        return _MdsApiSample_OnSnapshotFullRefresh(
                pSessionInfo, pMsgHead, &pRspMsg->mktDataSnapshot);

    case MDS_MSGTYPE_SECURITY_STATUS:
        /* 处理(深圳)证券状态消息 */
        return _MdsApiSample_OnSecurityStatus(
                pSessionInfo, pMsgHead, &pRspMsg->securityStatus);

    case MDS_MSGTYPE_TRADING_SESSION_STATUS:
        /* 处理(上证)市场状态消息 */
        return _MdsApiSample_OnTradingSessionStatus(
                pSessionInfo, pMsgHead, &pRspMsg->trdSessionStatus);

    case MDS_MSGTYPE_MARKET_DATA_REQUEST:
        /* 处理行情订阅请求的应答消息 */
        return _MdsApiSample_PrintMsg(pSessionInfo, pMsgHead,
                &pRspMsg->mktDataRequestRsp, stdout);

    case MDS_MSGTYPE_TEST_REQUEST:
        /* 处理测试请求的应答消息 */
        return _MdsApiSample_PrintMsg(pSessionInfo, pMsgHead,
                &pRspMsg->testRequestRsp, stdout);

    case MDS_MSGTYPE_HEARTBEAT:
        /* 直接忽略心跳消息即可 */
        break;

    default:
        SLOG_ERROR("无效的消息类型, 忽略之! msgId[0x%02X], server[%s:%d]",
                pMsgHead->msgId, pSessionInfo->channel.remoteAddr,
                pSessionInfo->channel.remotePort);
        return EFTYPE;
    }

    return 0;
}


/**
 * 超时检查处理
 *
 * @param   pSessionInfo    会话信息
 * @return  等于0，运行正常，未超时；大于0，已超时，需要重建连接；小于0，失败（错误号）
 */
static inline int32
_MdsApiSample_OnTimeout(MdsApiSessionInfoT *pSessionInfo) {
    /*
    int64               recvInterval = 0;

    SLOG_ASSERT(pSessionInfo);

    recvInterval = STime_GetSysTime() - MdsApi_GetLastRecvTime(pSessionInfo);
    if (unlikely(pSessionInfo->heartBtInt > 0
            && recvInterval > pSessionInfo->heartBtInt * 2)) {
        SLOG_ERROR("会话已超时, 将主动断开与服务器[%s:%d]的连接! " \
                "lastRecvTime[%lld], lastSendTime[%lld], " \
                "heartBtInt[%d], recvInterval[%lld]",
                pSessionInfo->channel.remoteAddr,
                pSessionInfo->channel.remotePort,
                MdsApi_GetLastRecvTime(pSessionInfo),
                MdsApi_GetLastSendTime(pSessionInfo),
                pSessionInfo->heartBtInt, recvInterval);
        return ETIMEDOUT;
    }
    */
    return 0;
}


/* ===================================================================
 * 行情接收处理(TCP/UDP)的示例代码
 * =================================================================== */

/**
 * TCP行情接收处理 (可以做为线程的主函数运行)
 *
 * @param   pTcpChannel     TCP行情订阅通道的会话信息
 * @return  TRUE 处理成功; FALSE 处理失败
 */
void*
MdsApiSample_TcpThreadMain(MdsApiSessionInfoT *pTcpChannel) {
   pthread_mutex_lock(&mutex); 
    static const int32  THE_TIMEOUT_MS = 5000;
    int32               ret = 0;

    SLOG_ASSERT(pTcpChannel);
    while(1){
        /* 等待行情消息到达, 并通过回调函数对消息进行处理 */
        ret = MdsApi_WaitOnMsg(pTcpChannel, THE_TIMEOUT_MS,
                _MdsApiSample_HandleMsg, NULL);
        if (unlikely(ret < 0)) {
            if (likely(SPK_IS_NEG_ETIMEDOUT(ret))) {
                }

            if (SPK_IS_NEG_EPIPE(ret)) {
            }
        }
    }
    usleep(200);
pthread_mutex_unlock(&mutex);
    return (void *) TRUE;
}

/**
 * 查询行情快照
 *
 * @param   pQryChannel     查询通道的会话信息
 * @param   exchangeId      交易所代码
 * @param   instrId         产品代码
 * @return  大于等于0，成功；小于0，失败（错误号）
 */
static inline int32
_MdsApiSample_QueryMktDataSnapshot(MdsApiSessionInfoT *pQryChannel,
        eMdsExchangeIdT exchangeId, int32 instrId) {
    MdsMktDataSnapshotT snapshot = {NULLOBJ_MDS_MKT_DATA_SNAPSHOT};
    int32               ret = 0;

    ret = MdsApi_QueryMktDataSnapshot(pQryChannel, exchangeId,
            MDS_SECURITY_TYPE_STOCK, instrId, &snapshot);
    if (unlikely(ret < 0)) {
        SLOG_ERROR("Query snapshot failure! " \
                "ret[%d], exchangeId[%" __SPK_FMT_HH__ "u], instrId[%d]",
                ret, (uint8) exchangeId, instrId);
        return ret;
    }

    SLOG_DEBUG("... Query snapshot success! " \
            "SecurityID[%s], TradePx[%d], updateTime[%09d]",
            snapshot.l2Stock.SecurityID, snapshot.l2Stock.TradePx,
            snapshot.head.updateTime);
    return 0;
}


/**
 * 查询线程的主函数 (可以做为线程的主函数运行)
 *
 * @param   pQryChannel     查询通道的会话信息
 * @return  TRUE 处理成功; FALSE 处理失败
 */
void*
MdsApiSample_QueryThreadMain(MdsApiSessionInfoT *pQryChannel) {
    int32               ret = 0;

    SLOG_ASSERT(pQryChannel);

    while (1) {
        ret = _MdsApiSample_QueryMktDataSnapshot(pQryChannel,
                MDS_EXCH_SSE, 601881);
        if (unlikely(ret < 0)) {
            goto ON_ERROR;
        }

        SPK_SLEEP_MS(10000);
    }

    return (void *) TRUE;

ON_ERROR:
    return (void *) FALSE;
}


/* ===================================================================
 * 负责建立连接和初始化的主函数
 * =================================================================== */

/**
 * API接口库示例程序的主函数
 */
int32
MdsApiSample_Main() {
    static const char   THE_CONFIG_FILE_NAME[] = "mds_client.conf";
    MdsApiClientEnvT    cliEnv = {NULLOBJ_MDSAPI_CLIENT_ENV};

    /*
     * 设置登录MDS时使用的用户名和密码
     * @note 如通过API接口设置，则可以不在配置文件中配置;
     */
    // MdsApi_SetThreadUsername("client1");
    // MdsApi_SetThreadPassword("123456");
    // MdsApi_SetThreadPassword("txt:123456");

    /* 初始化客户端环境 (配置文件参见: mds_client.conf) */
    if (! MdsApi_InitAll(&cliEnv, THE_CONFIG_FILE_NAME,
            MDSAPI_CFG_DEFAULT_SECTION_LOGGER, MDSAPI_CFG_DEFAULT_SECTION,
            MDSAPI_CFG_DEFAULT_KEY_TCP_ADDR, MDSAPI_CFG_DEFAULT_KEY_QRY_ADDR,
            (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL)) {
        return -1;
    }

    /* 直接在主线程内接收行情消息 (@note 实际场景中应该创建单独的行情接收线程)
    if (! MdsApiSample_TcpThreadMain(&cliEnv.tcpChannel)) {
        goto ON_ERROR;
    }
    */

    /* Linux 下的独立行情接收线程 */

   int rc;
   void *pool;
   int i;
  /* 
   rc = threadpool_create(&pool, 0, 30 , 200, 500);
    if (rc < 0) {
        SLOG_ERROR("threadpool_create false\n");
        goto ON_ERROR;
    }
    
    for(i = 0; i<20;i++){
        rc = threadpool_add_task(pool, (void* (*)(void *)) MdsApiSample_TcpThreadMain,&cliEnv.tcpChannel);    
        if(rc<0){
            SLOG_ERROR("threadpool_create false\n");
            goto ON_ERROR;
        }
    } 
*/
    pool_init(10);
    
    int ret = 0;
    for(i = 0; i < 10; i++)
    {
        ret = pool_add_worker((void* (*)(void *))MdsApiSample_TcpThreadMain, &cliEnv.tcpChannel); 
    }


    while(1)
    {
        sleep(100);
    }

    /* 关闭客户端环境, 释放会话数据 */
    MdsApi_LogoutAll(&cliEnv, TRUE);
    pool_destroy();
#ifdef LOG_INFO
    SLOG_INFO(">>> ...MdsApi_LogoutAll ...");
#endif

    return 0;

ON_ERROR:
    /* 直接断开与服务器的连接并释放会话数据 */
    MdsApi_DestoryAll(&cliEnv);
    pool_destroy();
    return -1;
}


int
main(int argc, char *argv[]) {
    return MdsApiSample_Main();
}
