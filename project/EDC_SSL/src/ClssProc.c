﻿/*
 * ============================================================================
 * COPYRIGHT
 *               PAX TECHNOLOGY, Inc. PROPRIETARY INFORMATION
 *   This software is supplied under the terms of a license agreement or
 *   nondisclosure agreement with PAX  Technology, Inc. and may not be copied
 *   or disclosed except in accordance with the terms in that agreement.
 *      Copyright (C) 2006-? PAX Technology, Inc. All rights reserved.
 * Description: ���彻�״���ģ��
 * Revision History:
 * Date                  Author                 Action
 * 2006.09.12         shengjx               created
 * ============================================================================
 */

#include "global.h"

#ifdef ENABLE_CONTLESS

/********************** Internal macros declaration ************************/
// macros for analyze EMV TLV string
#define TAGMASK_CLASS			0xC0	// tag mask of tag class
#define TAGMASK_CONSTRUCTED		0x20	// tag mask of constructed/primitive data
#define TAGMASK_FIRSTBYTE		0x1F	// tag mask of first byte
#define TAGMASK_NEXTBYTE		0x80	// tag mask of next byte

#define LENMASK_NEXTBYTE		0x80	// length mask
#define LENMASK_LENBYTES		0x7F	// mask of bytes of lenght

#define TAG_NULL_1				0x00	// null tag
#define TAG_NULL_2				0xFF	// null tag

#define DE55_MUST_SET			0x10	// 必须存在
#define DE55_OPT_SET			0x20	// 可选择存在
#define DE55_COND_SET			0x30	// 根据条件存在
/********************** Internal structure declaration *********************/

// callback function for GetTLVItem() to save TLV value
typedef void (*SaveTLVData)(uint uiTag, const uchar *psData, int iDataLen);

typedef struct _tagDE55Tag
{
	ushort	uiEmvTag;
	uchar	ucOption;
	uchar	ucLen;	
}DE55ClSSTag;

typedef struct _tagClssData
{
	uchar ucCardType;
	uchar ucSchemeId;

}CLSSDATA;

//added by Gillian Chen 2015/9/25
typedef struct
{
	unsigned char aucProgramId[17]; //Application Program ID
	unsigned char ucPrgramIdLen;    //Application Program ID锟斤拷锟斤拷
	Clss_PreProcInfo tPreProcDRL;
}App_ProgramID;

extern uchar g_aucOutcomeParamSet[8];
extern uchar g_aucUserInterReqData[22];
extern uchar g_aucErrIndication[6];
extern CLSS_DISC_DATA_MC g_tDiscretionayData;
static uchar g_ucDRLSupportFlg = 1;

Clss_PreProcInfo g_tPreProcInfo[MAX_APP_NUM];
Clss_ProgramID g_tProgInfo;

#define MAX_WAVE_AID_NUM 10//support WAVE ID count
CLSS_OUTCOME_DATA g_tOutComeData;

//added by Gillian Chen 2015/9/25
Clss_PreProcInfo g_tClssPreProcInfoIn;
uchar g_aucProID[17];
int g_iProIDLen = 0;
uchar g_ucRemovalTimeup = 0;

static CLSSDATA sg_tClssData;
EMV_CAPK g_tCAPKeyList[100];

static uchar sg_aucAuthData[16];			// authentication data from issuer
static uchar sg_aucIssuerScript[300];	// issuer script
static int sg_iAuthDataLen, sg_iScriptLen;

/********************** Internal functions declaration *********************/
int IsConstructedTag(uint uiTag);
static int  GetTLVItem(uchar **ppsTLVString, int iMaxLen, SaveTLVData pfSaveData, uchar bExpandAll);
int GetSpecTLVItem(uchar *psTLVString, int iMaxLen, uint uiSearchTag, uchar *psOutTLV, ushort *puiOutLen);
int Clss_transmit(uchar ucKerId);
int IssScrCon(void);
int Clss_SetTLVData(unsigned int uiTag, uchar *pucData, int iDatalen, uchar ucFlag);
int Clss_GetTLVData(unsigned int uiTag, uchar *pucData, int *piDatalen, uchar ucFlag);
int disp_clss_err(int iErr);

int ClssTransInit(void);
int nSetDETData(uchar *pucTag, uchar ucTagLen, uchar *pucData, uchar ucDataLen);
int SetClSSDE55(uchar bForUpLoad, uchar *psOutData, int *piOutLen);
int SetStdDEClSS55(uchar bForUpLoad, DE55ClSSTag *pstList, uchar *psOutData, int *piOutLen);
//uchar SearchSpecTLV(ushort nTag, uchar *sDataIn, ushort nDataInLen, uchar *sDataOut, ushort *pnDataOutLen);
void BuildCLSSTLVString(ushort uiEmvTag, uchar *psData, int iLength, uchar **ppsOutData);
int GetPanFromTrack2(uchar *pszPAN, uchar *pszExpDate,int iLen);//hdadd少了参数
//added by Gillian  2015/9/25
int nAppFindMatchProID(unsigned char *pucProID, int ucProIDLen);
//added by Kevinliu 2015/11/28
int Clss_CompleteTrans(void);
int ClssPreProcTxnParam(void);
int ClssCompleteTrans_AE(uchar ucInOnlineResult, uchar aucRspCode[], uchar aucAuthCode[], uchar aucIAuthData[], int nIAuthDataLen,  uchar aucScript[], int nScriptLen);
int ClssCompleteTrans_DPAS(uchar ucInOnlineResult, uchar aucRspCode[], uchar aucAuthCode[], uchar aucIAuthData[], int nIAuthDataLen,  uchar aucScript[], int nScriptLen);
int ClssCompleteTrans_JCB(uchar ucInOnlineResult, uchar aucRspCode[], uchar aucAuthCode[], uchar aucIAuthData[], int nIAuthDataLen,  uchar aucScript[], int nScriptLen);
int ClssCompleteTrans_WAVE(uchar ucInOnlineResult, uchar aucRspCode[], uchar aucAuthCode[], uchar aucIAuthData[], int nIAuthDataLen,  uchar aucScript[], int nScriptLen);

/********************** Internal variables declaration *********************/

// 非接消费55域标签,目前与EMV的标签一致
// F55 TLV format 
static DE55ClSSTag sgStdClssTagList[] =
{
	{0x57,   DE55_OPT_SET,  0},
	{0x5A,   DE55_OPT_SET,  0},
	{0x5F24, DE55_OPT_SET,  0},
	{0x5F2A, DE55_MUST_SET,  0},
	{0x5F34, DE55_OPT_SET,  0},
	{0x82,   DE55_MUST_SET,  0},
	{0x84,   DE55_MUST_SET,  0},
	{0x95,   DE55_MUST_SET,  0},
	{0x9A,   DE55_MUST_SET,  0},
	{0x9B,   DE55_OPT_SET,  0},
	{0x9C,   DE55_MUST_SET,  0},
	{0x9F02, DE55_MUST_SET,  0},
	{0x9F03, DE55_MUST_SET,  0},
	{0x9F08, DE55_OPT_SET,  0},
	{0x9F09, DE55_OPT_SET,  0},
	{0x9F10, DE55_OPT_SET,  0},
	{0x9F1A, DE55_MUST_SET,  0},
	{0x9F1E, DE55_OPT_SET,  0},
//	{0x9F1F, DE55_OPT_SET,  0},
	{0x9F26, DE55_MUST_SET,  0},
	{0x9F27, DE55_MUST_SET,  0},
	{0x9F33, DE55_MUST_SET,  0},
	{0x9F34, DE55_MUST_SET,  0},
	{0x9F35, DE55_OPT_SET,  0},
	{0x9F36, DE55_MUST_SET,  0},
	{0x9F37, DE55_MUST_SET,  0},
	{0x9F41, DE55_OPT_SET,  0},
	{0x9F5B, DE55_OPT_SET,  0},
	{0},
};

// TC-UPLOAD, TLV format
static DE55ClSSTag sgTcClssTagList[] =
{
	{0x5A,   DE55_OPT_SET,  0},
	{0x5F24, DE55_OPT_SET,  0},
	{0x5F2A, DE55_MUST_SET,  0},
	{0x5F34, DE55_OPT_SET,  0},
	{0x82,   DE55_MUST_SET,  0},
	{0x84,   DE55_MUST_SET,  0},
	{0x8A,   DE55_OPT_SET,  0},
	{0x95,   DE55_MUST_SET,  0},
	{0x9A,   DE55_MUST_SET,  0},
	{0x9B,   DE55_OPT_SET,  0},
	{0x9C,   DE55_MUST_SET,  0},
	{0x9F02, DE55_MUST_SET,  0},
	{0x9F03, DE55_MUST_SET,  0},
	{0x9F08, DE55_OPT_SET,  0},
	{0x9F09, DE55_OPT_SET,  0},
	{0x9F10, DE55_OPT_SET,  0},
	{0x9F18, DE55_OPT_SET,  0},
	{0x9F1A, DE55_MUST_SET,  0},
	{0x9F1E, DE55_OPT_SET,  0},
	{0x9F26, DE55_MUST_SET,  0},
	{0x9F27, DE55_MUST_SET,  0},
	{0x9F33, DE55_MUST_SET,  0},
	{0x9F34, DE55_MUST_SET,  0},
	{0x9F35, DE55_OPT_SET,  0},
	{0x9F36, DE55_MUST_SET,  0},
	{0x9F37, DE55_MUST_SET,  0},
	{0x9F41, DE55_OPT_SET,  0},
	{0x9F5B, DE55_OPT_SET,  0},
	{0},
};

// 非接消费56域标签,目前与EMV的标签一致
// F56 TLV format 
static DE55ClSSTag sgStdClssField56TagList[] =
{
	{0x5A,   DE55_MUST_SET, 0},
	{0x95,   DE55_MUST_SET, 0},
	{0x9B,   DE55_MUST_SET, 0},
	{0x9F10, DE55_MUST_SET, 0},
	{0x9F26, DE55_MUST_SET, 0},
	{0x9F27, DE55_MUST_SET, 0},
	{0},
};

/********************** external reference declaration *********************/

/******************>>>>>>>>>>>>>Implementations<<<<<<<<<<<<*****************/
//----------------------------------------------------------------------------------
//                                 
//                                     L3回调函数 
//									   L3Callback functions
//
//-----------------------------------------------------------------------------------


int SetClSSDE55(uchar bForUpLoad, uchar *psOutData, int *piOutLen)
{
	if (bForUpLoad)
	{
		return SetStdDEClSS55(bForUpLoad, sgTcClssTagList, psOutData, piOutLen);
	}
	else
	{
		return SetStdDEClSS55(bForUpLoad, sgStdClssTagList, psOutData, piOutLen);
	}
}


// set ADVT/TIP bit 55
int SetStdDEClSS55(uchar bForUpLoad, DE55ClSSTag *pstList, uchar *psOutData, int *piOutLen)
{
	int		iRet, iCnt;
	int 	iLength;
	uchar	*psTemp, sBuff[200];

	*piOutLen = 0;
	psTemp    = psOutData;

	if (sg_tClssData.ucSchemeId==CLSS_MC_MAG)
	{
		return 0;
	}

	for(iCnt=0; pstList[iCnt].uiEmvTag!=0; iCnt++)
	{
		memset(sBuff, 0, sizeof(sBuff));
		//在非接触L2 的qPBOC及payWave中,'终端性能(9F33)'数据元无法从这两个库中获取。
		if (pstList[iCnt].uiEmvTag == 0x9F33)
		{
			EMVGetParameter(&glEmvParam);
			memcpy(sBuff, glEmvParam.Capability, 3);
			iLength = 3;
			BuildCLSSTLVString(pstList[iCnt].uiEmvTag, sBuff, iLength, &psTemp);
		}
		else
		{
			iRet = Clss_GetTLVData(pstList[iCnt].uiEmvTag, sBuff, &iLength, sg_tClssData.ucCardType);
			if( iRet==EMV_OK )
			{
				BuildCLSSTLVString(pstList[iCnt].uiEmvTag, sBuff, iLength, &psTemp);
			}
			else if( pstList[iCnt].ucOption==DE55_MUST_SET )
			{
				BuildCLSSTLVString(pstList[iCnt].uiEmvTag, NULL, 0, &psTemp);
			}
		}
	}

	if( glProcInfo.stTranLog.szPan[0]=='5' )
	{	// for master card TCC = "R" -- retail
		BuildCLSSTLVString(0x9F53, (uchar *)"R", 1, &psTemp);

		if (bForUpLoad)
		{
			memset(sBuff, 0, sizeof(sBuff));
			iRet = Clss_GetTLVData(0x91, sBuff, &iLength, sg_tClssData.ucCardType);
			if( iRet==EMV_OK )
			{
				BuildCLSSTLVString(0x91, sBuff, iLength, &psTemp);
			}
		}
	}

	*piOutLen = (psTemp-psOutData);

	return 0;
}

// Save Iuuser Authentication Data, Issuer Script.
void SaveRspICCData(uint uiTag, const uchar *psData, int iDataLen)
{
	switch( uiTag )
	{
	case 0x91:
		memcpy(sg_aucAuthData, psData, MIN(iDataLen, 16));
		sg_iAuthDataLen = MIN(iDataLen, 16);

		OsLog(LOG_INFO, "%s - %d, sg_aucAuthData = %s", __FUNCTION__, __LINE__, sg_aucAuthData);
		OsLog(LOG_INFO, "%s - %d, sg_iAuthDataLen = %d", __FUNCTION__, __LINE__, sg_iAuthDataLen);
		break;
	case 0x71:
	case 0x72:
		sg_aucIssuerScript[sg_iScriptLen++] = (uchar)uiTag;
		if( iDataLen > 127 )
		{
			sg_aucIssuerScript[sg_iScriptLen++] = 0x81;
		}
		sg_aucIssuerScript[sg_iScriptLen++] = (uchar)iDataLen;
		memcpy(&sg_aucIssuerScript[sg_iScriptLen], psData, iDataLen);
		sg_iScriptLen += iDataLen;

		OsLog(LOG_INFO, "%s - %d, sg_aucIssuerScript = %s", __FUNCTION__, __LINE__, sg_aucIssuerScript);
		OsLog(LOG_INFO, "%s - %d, sg_iScriptLen = %d", __FUNCTION__, __LINE__, sg_iScriptLen);
		break;

	case 0x9F36:
//		memcpy(sATC, psData, MIN(iDataLen, 2));	// ignore
		break;

	default:
		break;
	}
}

// 只处理基本数据元素Tag,不包括结构/模板类的Tag
// Build Clss basic TLV data, exclude structure/template.
void BuildCLSSTLVString(ushort uiEmvTag, uchar *psData, int iLength, uchar **ppsOutData)
{
	uchar	*psTemp = NULL;

	if( iLength<0 )
	{
		return;
	}

	// 设置TAG
	// write tag
	psTemp = *ppsOutData;
	if( uiEmvTag & 0xFF00 )
	{
		*psTemp++ = (uchar)(uiEmvTag >> 8);
	}
	*psTemp++ = (uchar)uiEmvTag;

	// 设置Length
	// write length
	if( iLength<=127 )	// 目前数据长度均小余127字节,但仍按标准进行处理
	{
		*psTemp++ = (uchar)iLength;
	}
	else
	{	// EMV规定最多255字节的数据
		*psTemp++ = 0x81;
		*psTemp++ = (uchar)iLength;
	}

	// 设置Value
	// write value
	if( iLength>0 )
	{
		memcpy(psTemp, psData, iLength);
		psTemp += iLength;
	}

	*ppsOutData = psTemp;
}

// 从2磁道信息分析出卡号(PAN)
int GetPanFromTrack2(uchar *pszPAN, uchar *pszExpDate, int iLen)
{
	int		iPanLen = 0;
	char	*p = NULL, pszTemp[41] = {0};
	
	// 从2磁道开始到'D'
// 	iPanLen = glProcInfo.szTrack2;
// 	if( iPanLen>0 )
// 	{
// 		memset(pszTemp, 0, sizeof(pszTemp));
// 		PubBcd2Asc0(glProcInfo.szTrack2, iPanLen, pszTemp);
// 	}
// 	else
// 	{	// 2磁道都没有
// 		return ERR_SWIPECARD;
// 	}
	memset(pszTemp, 0, sizeof(pszTemp));
	memcpy(pszTemp, glProcInfo.szTrack2, iLen);

	
	p = strchr((char *)pszTemp, '=');
	if( p==NULL )
	{
		return ERR_SWIPECARD;
	}
	iPanLen = strlen(pszTemp) - strlen(p);
	if( iPanLen<13 || iPanLen>19 )
	{
		return ERR_SWIPECARD;
	}
	
	sprintf((char *)pszPAN, "%.*s", iPanLen, pszTemp);
	sprintf((char *)pszExpDate, "%.4s", p+1);
	
	return 0;
}

//callback function
//added by kevinliu 2015/11/24
int Clss_cSetCAPKandRevoc_AE(unsigned char ucIndex, unsigned char *pucRid)
{
	int i = 0, iRet = 0;
	EMV_REVOCLIST tRevocList;

	if(pucRid == NULL)
	{
		return EMV_PARAM_ERR;
	}

	for (i = 0; i < glSysParam.uiCapkNum; i++)
	{
		if ((g_tCAPKeyList[i].KeyID == ucIndex) && (0 == strcmp(g_tCAPKeyList[i].RID, pucRid)))
		{
			iRet = Clss_AddCAPK_AE(&g_tCAPKeyList[i]);
			if (EMV_OK != iRet)
			{
				return EMV_PARAM_ERR;
			}
			break;
		}
	}
	if (i == glSysParam.uiCapkNum)	//no one match
	{
		return EMV_PARAM_ERR;
	}

	memset(&tRevocList, 0, sizeof(EMV_REVOCLIST));
	memcpy(tRevocList.ucRid, pucRid, 5);
	tRevocList.ucIndex = ucIndex;
	memcpy(tRevocList.ucCertSn, "\x00\x07\x11", 3);
	Clss_AddRevocList_AE(&tRevocList);

	return EMV_OK;
}

int ClssDetectTapCard(void)
{
	int iRet = 0, iTime = 0;
	uchar ucKey = 0;

	//tap card
	iRet = PiccOpen();
	if(iRet != 0)
	{
		SetClssLightStatus(CLSSLIGHTSTATUS_ERROR);
		Gui_ClearScr();
		Gui_ShowMsgBox(GetCurrTitle(), gl_stTitleAttr, _ICON_FAIL, _T("OPEN PICC ERR"), gl_stCenterAttr, GUI_BUTTON_CANCEL, 30, NULL);
		return ERR_NO_DISP;
	}
	TimerSet(3,500);
	kbflush();
	while(1)
	{
		iTime = TimerCheck(3);
		if(!iTime)
		{
			return ERR_TIMEOUT; // EPT-17
		}

		if(kbhit() != NOKEY)
		{
			ucKey = getkey();
			if(ucKey == KEYCANCEL)
			{
				return ERR_USERCANCEL;
			}
		}
		else	//modified by kevinliu 2015/12/09 It's hard to cancel before modified.
		{
			iRet = PiccDetect(0, NULL, NULL, NULL, NULL);
			if(iRet == 0)
			    break;
			else if(iRet == 3|| iRet==5 || iRet==6)
			{
				DelayMs(100);
				continue;
			}
			else if(iRet == 4)
			{
				SetClssLightStatus(CLSSLIGHTSTATUS_ERROR);
				Gui_ClearScr();
				Gui_ShowMsgBox(GetCurrTitle(), gl_stTitleAttr, _ICON_WARNING, _T("TOO MANY CARD"), gl_stCenterAttr, GUI_BUTTON_CANCEL, 30, NULL);
				return ERR_NO_DISP;
			}
			else// if(iRet == 1 || iRet==2)
            {
                return ERR_TRAN_FAIL;
            }
		}
	}
	return 0;
}

int ClssProcFlow_VISA(uchar *pucfinalData, int iFinalDataLen, Clss_PreProcInterInfo tClssProcInterInfo)
{
#ifdef CLSS_DEBUG
Clss_SetDebug_Wave(1);
#endif
	int iRet = 0;
	uchar ucAcType = 0, ucPathType = 0, ucTemp[100] = {0};
	Clss_TransParam tClssTransParam;

	memset(&tClssTransParam, 0, sizeof(Clss_TransParam));
	GetClssTxnParam(&tClssTransParam);

	ClssBaseParameterSet_WAVE();
	vSetFinalSelectAID(pucfinalData+1, pucfinalData[0]);
	iRet = Clss_SetFinalSelectData_Wave(pucfinalData, iFinalDataLen);
	if(iRet != EMV_OK)
	{
		return iRet;
	}

	//added by Gillian Chen 2015/9/25
	iRet = Clss_SetTLVData_Wave(0x9F5A, "123", 10);
	if(g_ucDRLSupportFlg == 1)
	{
		if (Clss_GetTLVData_Wave(0x9F5A, g_aucProID, &g_iProIDLen) == EMV_OK)
		{
			if (!nAppFindMatchProID(g_aucProID, g_iProIDLen))
			{
				if(Clss_SetDRLParam_Wave(g_tProgInfo) != EMV_OK)
				{
					return ERR_TRAN_FAIL;
				}
			}
			else
			{
				return ERR_TRAN_FAIL;
			}
		}
		else
		{
			return ERR_TRAN_FAIL;
		}
	}
	iRet = Clss_SetTransData_Wave(&tClssTransParam, &tClssProcInterInfo);
	if(iRet != EMV_OK)
	{
		return iRet;
	}

	ucAcType = 0;
	memset(ucTemp, 0, sizeof(ucTemp));
	iRet = Clss_Proctrans_Wave(ucTemp, &ucAcType);
	sg_tClssData.ucSchemeId = ucTemp[0];
	ucPathType = ucTemp[0];
	if(iRet)
	{
		if(iRet == CLSS_RESELECT_APP)
		{
			iRet = Clss_DelCurCandApp_Entry();
			if (iRet)
			{
				return iRet;
			}
			return iRet;
		}
		//see phone
		else if((iRet == CLSS_REFER_CONSUMER_DEVICE) && ((tClssProcInterInfo.aucReaderTTQ[0] & 0x20) == 0x20))
		{
			Inter_DisplayMsg(MSG_SEE_PHONE);
			iRet= App_Try_Again;
			DelayMs(1200);
			return iRet;
		}
		else if(iRet == CLSS_USE_CONTACT)
		{
			Inter_DisplayMsg(MSG_TRY_ANOTHER_INTERFACE);
			return CLSS_USE_CONTACT;
		}
		else
		{
			return iRet;
		}
	}

	vAppSetTransPath(ucPathType);

	iRet = nAppTransProc_VISA(ucPathType, ucAcType);

	return iRet;
}

int ClssProcFlow_MC(uchar *pucfinalData, int iFinalDataLen, Clss_PreProcInterInfo tClssProcInterInfo)
{
#ifdef CLSS_DEBUG
Clss_SetDebug_MC(1);
#endif
	int iRet = 0;
	uchar ucAcType = 0, ucPathType = 0;

	Clss_SetCBFun_SendTransDataOutput_MC(cClssSendTransDataOutput_MC);
	vSetFinalSelectAID(pucfinalData+1, pucfinalData[0]);
//	SetTermParam_MC();
	ClssTermParamSet_MC();
	nAppLoadTornLog_MC();	//added by Kevin Liu 20160728
	iRet = Clss_SetFinalSelectData_MC(pucfinalData, iFinalDataLen);
	// 返回值的判断，If the return code is not EMV_OK, Application should get DF8129). [12/29/2014 jiangjy]
	if(iRet == CLSS_RESELECT_APP)
	{
		iRet = Clss_DelCurCandApp_Entry();
		if (iRet != 0)
		{
			vInitPaymentData();
			g_aucUserInterReqData[0]=MI_ERROR_OTHER_CARD;
			g_aucUserInterReqData[1]=MI_NOT_READY;
			memcpy(g_aucUserInterReqData+2, MSG_HOLD_TIME_VALUE, 3);

			g_aucErrIndication[5]=MI_ERROR_OTHER_CARD;
			g_aucErrIndication[1] = L2_EMPTY_CANDIDATE_LIST;//S51.11
			g_aucOutcomeParamSet[0] = OC_END_APPLICATION;
			g_tDiscretionayData.ucErrIndicFlg = 1;
			//					nSendTransDataOutput_MC(T_UIRD | T_OCPS | T_DISD);//S51.11 S51.12
			return iRet;
		}
		return iRet;
	}
	else if(iRet)
	{
		return iRet;
	}
	iRet = Clss_InitiateApp_MC();
	//返回值的判断 返回值的判断，If the return code is not EMV_OK, Application should get DF8129)
	if (iRet == CLSS_RESELECT_APP) // GPO
	{
		iRet = Clss_DelCurCandApp_Entry();
		if (iRet)
		{
			vInitPaymentData();// paypass 3.0.1 by zhoujie
			g_aucUserInterReqData[0]=MI_ERROR_OTHER_CARD;//S51.11 for paypass 3.0.1 by zhoujie
			g_aucUserInterReqData[1]=MI_NOT_READY;
			memcpy(g_aucUserInterReqData+2, MSG_HOLD_TIME_VALUE, 3);

			g_aucErrIndication[5]=MI_ERROR_OTHER_CARD;
			g_aucErrIndication[1] = L2_EMPTY_CANDIDATE_LIST;//S51.11
			g_aucOutcomeParamSet[0] = OC_END_APPLICATION;
			g_tDiscretionayData.ucErrIndicFlg = 1;
			//					nSendTransDataOutput_MC(T_UIRD | T_OCPS | T_DISD);//S51.11 S51.12
			return iRet;
		}
		return iRet;
	}
	else if(iRet)
	{
		return iRet;
	}
	ucPathType = 0;
	iRet = Clss_ReadData_MC(&ucPathType);
	//返回值的判断，If the return code is not EMV_OK, Application should get DF8129)
	if(iRet)
	{
		if(iRet == CLSS_RESELECT_APP)
		{
			iRet = Clss_DelCurCandApp_Entry();
			if (iRet)
			{
				vInitPaymentData();
				g_aucUserInterReqData[0]=MI_ERROR_OTHER_CARD;
				g_aucUserInterReqData[1]=MI_NOT_READY;
				memcpy(g_aucUserInterReqData+2, MSG_HOLD_TIME_VALUE, 3);

				g_aucErrIndication[5]=MI_ERROR_OTHER_CARD;
				g_aucErrIndication[1] = L2_EMPTY_CANDIDATE_LIST;//S51.11
				g_aucOutcomeParamSet[0] = OC_END_APPLICATION;
				g_tDiscretionayData.ucErrIndicFlg = 1;

				return iRet;
			}
			return iRet;
		}
		else
		{
			return iRet;
		}
	}

	vAppSetTransPath(ucPathType);

	iRet = nAppTransProc_MC(ucPathType, &ucAcType);
	if (g_aucUserInterReqData[0] == MI_SEE_PHONE)
	{
		Inter_DisplayMsg(MSG_SEE_PHONE);
	}
	if (g_aucOutcomeParamSet[0] == OC_TRY_AGAIN || g_aucOutcomeParamSet[1] != OC_NA)
	{
		iRet = App_Try_Again;
	}

	return iRet;
}
//added by Kevinliu 2015-12-07
int ClssProcFlow_AE(uchar *pucfinalData, int iFinalDataLen, Clss_PreProcInterInfo tClssProcInterInfo)
{
#ifdef CLSS_DEBUG
Clss_SetDebug_AE(1);
#endif
	int iRet = 0;
	uchar ucAcType = 0, ucTemp[100] = {0}, ucPathType = 0;
	Clss_TransParam tClssTransParam;

	memset(&tClssTransParam, 0, sizeof(Clss_TransParam));
	GetClssTxnParam(&tClssTransParam);
	ClssBaseParameterSet_AE();
	vSetFinalSelectAID(pucfinalData+1, pucfinalData[0]);
	//returns	EMV_OK	CLSS_PARAM_ERR	EMV_DATA_ERR	CLSS_API_ORDER_ERR
	iRet = Clss_SetFinalSelectData_AE(pucfinalData, iFinalDataLen);
	if (iRet != EMV_OK)
	{
		return iRet;
	}
	iRet = Clss_SetTransData_AE(&tClssTransParam, &tClssProcInterInfo);
	//returns	EMV_OK	CLSS_PARAM_ERR
	if (iRet != EMV_OK)
	{
		return iRet;
	}

	memset(ucTemp, 0, sizeof(ucTemp));
	iRet = Clss_Proctrans_AE(&ucPathType);
	//returns	EMV_OK	CLSS_PARAM_ERR	CLSS_RESELECT_APP	CLSS_API_ORDER_ERR OTHER
	sg_tClssData.ucSchemeId = ucPathType;
	if(iRet)
	{
		if (iRet == CLSS_RESELECT_APP) // GPO
		{
			iRet = Clss_DelCurCandApp_Entry();
			//returns	EMV_OK
			if (iRet)
			{
				return iRet;
			}
			return iRet;
		}
		else
		{
			return iRet;
		}
	}

	vAppSetTransPath(ucPathType);
	iRet = nAppTransProc_AE(ucPathType, &ucAcType);
	//TODO
    if ((iRet == CLSS_REFER_CONSUMER_DEVICE) && ((tClssProcInterInfo.aucReaderTTQ[0] & 0x20) == 0x20))
    {
        Inter_DisplayMsg(MSG_SEE_PHONE);
        iRet= App_Try_Again;
    }

	return iRet;
}
//added by Kevinliu 2015-12-07
int ClssProcFlow_JCB(uchar *pucfinalData, int iFinalDataLen, Clss_PreProcInterInfo tClssProcInterInfo)
{
	int iRet = 0;
	uchar ucAcType = 0, ucPathType = 0;

	vSetFinalSelectAID(pucfinalData+1, pucfinalData[0]);
//	SetTermParam_JCB();
	ClssTermParamSet_JCB();
	ClssTxnParamSet_JCB();
	iRet = Clss_SetFinalSelectData_JCB(pucfinalData, iFinalDataLen);
	if(iRet == CLSS_RESELECT_APP)
	{
		iRet = Clss_DelCurCandApp_Entry();
		if (iRet != 0)
		{
			vInitPaymentData();
			g_aucUserInterReqData[0]=MI_ERROR_OTHER_CARD;
			g_aucUserInterReqData[1]=MI_NOT_READY;
			memcpy(g_aucUserInterReqData+2, MSG_HOLD_TIME_VALUE, 3);

			g_aucErrIndication[5]=MI_ERROR_OTHER_CARD;
			g_aucErrIndication[1] = L2_EMPTY_CANDIDATE_LIST;//S51.11
			g_aucOutcomeParamSet[0] = OC_END_APPLICATION;
			g_tDiscretionayData.ucErrIndicFlg = 1;

			return iRet;
		}
		return iRet;
	}
	else if(iRet)
	{
		return iRet;
	}

	ucPathType = 0;
	iRet = Clss_InitiateApp_JCB(&ucPathType);
	if (iRet == CLSS_RESELECT_APP) // GPO
	{
		iRet = Clss_DelCurCandApp_Entry();
		if (iRet)
		{
			vInitPaymentData();
			g_aucUserInterReqData[0]=MI_ERROR_OTHER_CARD;
			g_aucUserInterReqData[1]=MI_NOT_READY;
			memcpy(g_aucUserInterReqData+2, MSG_HOLD_TIME_VALUE, 3);

			g_aucErrIndication[5]=MI_ERROR_OTHER_CARD;
			g_aucErrIndication[1] = L2_EMPTY_CANDIDATE_LIST;
			g_aucOutcomeParamSet[0] = OC_END_APPLICATION;
			g_tDiscretionayData.ucErrIndicFlg = 1;
//			return ERR_TRAN_FAIL;
			return iRet;
		}
		return iRet;
	}
	else if(iRet)
	{
		return iRet;
	}

	iRet = Clss_ReadData_JCB();
	if(iRet)
	{
		if(iRet == CLSS_RESELECT_APP)
		{
			iRet = Clss_DelCurCandApp_Entry();
			if (iRet)
			{
				vInitPaymentData();
				g_aucUserInterReqData[0]=MI_ERROR_OTHER_CARD;
				g_aucUserInterReqData[1]=MI_NOT_READY;
				memcpy(g_aucUserInterReqData+2, MSG_HOLD_TIME_VALUE, 3);

				g_aucErrIndication[5]=MI_ERROR_OTHER_CARD;
				g_aucErrIndication[1] = L2_EMPTY_CANDIDATE_LIST;//S51.11
				g_aucOutcomeParamSet[0] = OC_END_APPLICATION;
				g_tDiscretionayData.ucErrIndicFlg = 1;

				return iRet;
			}
			return iRet;
		}
		else
		{
			return iRet;
		}
	}

	vAppSetTransPath(ucPathType);

	iRet = nAppTransProc_JCB(ucPathType, &ucAcType);
	if (g_aucUserInterReqData[0] == MI_SEE_PHONE)
	{
		Inter_DisplayMsg(MSG_SEE_PHONE);
	}
	if (g_aucOutcomeParamSet[0] == OC_TRY_AGAIN || g_aucOutcomeParamSet[1] != OC_NA)
	{
		iRet = App_Try_Again;
	}

	return iRet;
}

int ClssProcFlow_ZIP(uchar *pucfinalData, int iFinalDataLen, Clss_PreProcInterInfo tClssProcInterInfo)
{
	int iRet = 0;
	uchar ucAcType = 0, ucPathType = 0;

	vSetFinalSelectAID(pucfinalData+1, pucfinalData[0]);
	ClssTermParamSet_DPAS();
	iRet = Clss_SetFinalSelectData_DPAS(pucfinalData, iFinalDataLen);
	if(iRet)
	{
		cSendTransDataOutput_DPAS(0x07);
	}
	if(iRet == CLSS_RESELECT_APP)
	{
		iRet = Clss_DelCurCandApp_Entry();
		if (iRet != 0)
		{
			vInitPaymentData();
			g_aucUserInterReqData[0]=MI_ERROR_OTHER_CARD;
			g_aucUserInterReqData[1]=MI_NOT_READY;
			memcpy(g_aucUserInterReqData+2, MSG_HOLD_TIME_VALUE, 3);

			g_aucErrIndication[5]=MI_ERROR_OTHER_CARD;
			g_aucErrIndication[1] = L2_EMPTY_CANDIDATE_LIST;//S51.11
			g_aucOutcomeParamSet[0] = OC_END_APPLICATION;
			g_tDiscretionayData.ucErrIndicFlg = 1;
			//					nSendTransDataOutput_MC(T_UIRD | T_OCPS | T_DISD);//S51.11 S51.12
			return iRet;
		}
		return iRet;
	}
	else if(iRet)
	{
		return iRet;
	}
	//set 9F66 95 after Clss_SetFinalSelectData_DPAS
	if(memcmp(tClssProcInterInfo.aucAID, "\xA0\x00\x00\x03\x24",5) != 0)// D-PAS need set TTQ
	{
		iRet = nSetDETData((uchar*)"\x9F\x66", 2, tClssProcInterInfo.aucReaderTTQ, 4);
	}
	if(tClssProcInterInfo.ucRdCLFLmtExceed == 1)
	{
		iRet = nSetDETData((uchar*)"\x95", 1, (uchar*)"\x00\x40\x04\x80\x00", 5);	//why? modified by kevinliu
//		iRet = nSetDETData((uchar*)"\x95", 1, (uchar*)"\x00\x00\x00\x80\x00", 5);
	}
	//set dynamic transaction parameters
	ClssTxnParamSet_DPas();
	iRet = Clss_InitiateApp_DPAS(&ucPathType);
	if(iRet)
	{
		cSendTransDataOutput_DPAS(0x07);
		if(iRet == CLSS_RESELECT_APP)
		{
			iRet = Clss_DelCurCandApp_Entry();
			if (iRet != 0)
			{
				vInitPaymentData();
				g_aucUserInterReqData[0]=MI_ERROR_OTHER_CARD;
				g_aucUserInterReqData[1]=MI_NOT_READY;
				memcpy(g_aucUserInterReqData+2, MSG_HOLD_TIME_VALUE, 3);

				g_aucErrIndication[5]=MI_ERROR_OTHER_CARD;
				g_aucErrIndication[1] = L2_EMPTY_CANDIDATE_LIST;//S51.11
				g_aucOutcomeParamSet[0] = OC_END_APPLICATION;
				g_tDiscretionayData.ucErrIndicFlg = 1;
				//					nSendTransDataOutput_MC(T_UIRD | T_OCPS | T_DISD);//S51.11 S51.12
				return iRet;
			}
			return iRet;
		}
		else
		{
			return iRet;
		}
	}

	vAppSetTransPath(ucPathType);

	//added by Gillian 2015/12/17
	iRet = nAppTransProc_DPAS(ucPathType, &ucAcType);
	if (g_aucUserInterReqData[0] == MI_SEE_PHONE)
	{
		Inter_DisplayMsg(MSG_SEE_PHONE);
	}
	if (g_aucOutcomeParamSet[0] == OC_TRY_AGAIN || g_aucOutcomeParamSet[1] != OC_NA)
	{
		iRet = App_Try_Again;
	}
	return iRet;
}


//added by Kevinliu 2015-12-07
int ClssProcFlow_ALL(void)
{
	int	 iRet = 0, iLen = 0;
	uchar ucTemp[300] = {0}, ucKernType = 0;
	Clss_PreProcInterInfo tClssProcInterInfo;
	Clss_TransParam tClssTransParam;
	uchar	szTotalAmt[12+1]={0};

	GetClssTxnParam(&tClssTransParam);
	//display price and tap card prompt.
	memset(ucTemp, 0, sizeof(ucTemp));
	memset(szTotalAmt, 0, sizeof(szTotalAmt));
	PubAscAdd(glProcInfo.stTranLog.szAmount, glProcInfo.stTranLog.szTipAmount, 12, szTotalAmt);
	PubConvAmount(glSysParam.stEdcInfo.stLocalCurrency.szName, szTotalAmt,
				glProcInfo.stTranLog.stTranCurrency.ucDecimal,
				glProcInfo.stTranLog.stTranCurrency.ucIgnoreDigit,
				ucTemp, 0);

	SetClssLightStatus(CLSSLIGHTSTATUS_READYFORTXN);
	strcat((char*)ucTemp, "\n");
	strcat((char*)ucTemp, _T("PLS TAP CARD"));

	Gui_ClearScr();
	Gui_ShowMsgBox(GetCurrTitle(), gl_stTitleAttr, NULL, ucTemp, gl_stCenterAttr, GUI_BUTTON_NONE, 0,NULL);

	//detect tap card
	iRet = ClssDetectTapCard();
	//returns ERR_USERCANCEL	ERR_TRAN_FAIL
	if(iRet)
	{
		return iRet;
	}

	SetClssLightStatus(CLSSLIGHTSTATUS_PROCESSING);
	Gui_ClearScr();
	Gui_ShowMsgBox(GetCurrTitle(), gl_stTitleAttr, NULL, _T("PLS WAIT..."), gl_stCenterAttr, GUI_BUTTON_NONE, 0, NULL);

	glProcInfo.stTranLog.uiEntryMode = MODE_CONTACTLESS;
	Clss_SetMCVersion_Entry(3);// add for paypass 3.0 [12/29/2014 jiangjy]
	//app select
	iRet = Clss_AppSlt_Entry(0,0);
	//returns	EMV_OK	CLSS_PARAM_ERR	ICC_CMD_ERR	ICC_BLOCK	EMV_NO_APP	EMV_APP_BLOCK	EMV_NO_APP_PPSE_ERR
	if(iRet != EMV_OK)
	{
		vAppCreateOutcomeData_MC(iRet);
		return iRet;
	}

	while(1)
	{
		vAppInitPaymentData_MC();
		ucKernType = 0;
		iLen = 0;

		memset(ucTemp, 0, sizeof(ucTemp));
		iRet = Clss_FinalSelect_Entry(&ucKernType, ucTemp, &iLen);
		//returns	EMV_OK	CLSS_PARAM_ERR	ICC_CMD_ERR	EMV_RSP_ERR	EMV_NO_APP	EMV_APP_BLOCK	ICC_BLOCK
		//CLSS_USE_CONTACT	EMV_DATA_ERR	CLSS_RESELECT_APP
		if(iRet != EMV_OK)
		{
			return iRet;
		}
		//VISA MASTERCARD AMERICANEXPRESS JCB DISCOVER
		if(ucKernType != KERNTYPE_VIS && ucKernType != KERNTYPE_MC && ucKernType != KERNTYPE_AE
				&& ucKernType != KERNTYPE_JCB && ucKernType != KERNTYPE_ZIP)
		{
			Gui_ClearScr();
			Gui_ShowMsgBox(GetCurrTitle(), gl_stTitleAttr, _ICON_FAIL, _T("UNSUPPORT CTLS"), gl_stCenterAttr, GUI_BUTTON_CANCEL, 30, NULL);
			return ERR_TRAN_FAIL;
		}

		//get pre-process data
		memset(&tClssProcInterInfo, 0, sizeof(Clss_PreProcInterInfo));
		iRet = Clss_GetPreProcInterFlg_Entry(&tClssProcInterInfo);
		//returns	EMV_OK	CLSS_PARAM_ERR	CLSS_USE_CONTACT
		if(iRet != EMV_OK)
		{
			return iRet;
		}
		iLen = 0;
		memset(ucTemp, 0, sizeof(ucTemp));
		iRet = Clss_GetFinalSelectData_Entry(ucTemp, &iLen);
		//returns	EMV_OK	CLSS_PARAM_ERR
		if(iRet != EMV_OK) 
		{
			return iRet;
		}

		iRet = nAppSetCurAppType(ucKernType);//CoreInit  //kernel type// scheme id

		switch(ucAppGetAppType())
		{
		case KERNTYPE_VIS:
			iRet = ClssProcFlow_VISA(ucTemp, iLen, tClssProcInterInfo);
			break;
		case KERNTYPE_MC:
			iRet = ClssProcFlow_MC(ucTemp, iLen, tClssProcInterInfo);
			break;
		case KERNTYPE_AE:
			iRet = ClssProcFlow_AE(ucTemp, iLen, tClssProcInterInfo);
			break;
		case KERNTYPE_JCB:
			iRet = ClssProcFlow_JCB(ucTemp, iLen, tClssProcInterInfo);
			break;
		case KERNTYPE_ZIP:
			iRet = ClssProcFlow_ZIP(ucTemp, iLen, tClssProcInterInfo);
			break;
		default:
			return ERR_TRAN_FAIL;
		}

		if(iRet == EMV_OK) {
		    break;
		}
		else if(iRet == CLSS_RESELECT_APP){
			continue;
		}
		else {
            appRemovePicc();
			break;
		}
	}
	return iRet;
}

int ClssPreProcTxnParam(void)
{
	int iRet = 0;
	Clss_TransParam tClssTransParam;

	//pre-process
	memset(&tClssTransParam, 0, sizeof(Clss_TransParam));
	tClssTransParam.ulAmntAuth = atol((char *)glProcInfo.stTranLog.szAmount) + atol((char *)glProcInfo.stTranLog.szTipAmount);
	tClssTransParam.ulAmntOther = 0;
	tClssTransParam.ucTransType = 0x00;//EMV_GOODS;
	PubAsc2Bcd(glProcInfo.stTranLog.szDateTime+2, 6, tClssTransParam.aucTransDate);
	PubAsc2Bcd(glProcInfo.stTranLog.szDateTime+8, 6, tClssTransParam.aucTransTime);
	tClssTransParam.ulTransNo = glProcInfo.stTranLog.ulSTAN;

	SetClssTxnParam(&tClssTransParam);
	iRet = Clss_PreTransProc_Entry(&tClssTransParam);
	//returns EMV_OK	CLSS_PARAM_ERR		CLSS_USE_CONTACT
	if(iRet != EMV_OK)
	{
		if (iRet == CLSS_USE_CONTACT)
		{
			disp_clss_err(iRet);
		}
	}
	return iRet;

}

//For CLSS light control added by kevinliu
//*********************************************************************************************************
static CLSSLIGHTSTATUS sg_eClssLightStatus;
static CLSSLIGHTSTATUS sg_ePrevStatus;
static int sg_fd_blueLED=-1;
static int sg_fd_yellowLED=-1;
static int sg_fd_greenLED=-1;
static int sg_fd_redLED=-1;
static int sg_fd_logoLED=-1;

void SetClssLightStatus(CLSSLIGHTSTATUS eStatus)
{
    sg_eClssLightStatus = eStatus;
}

CLSSLIGHTSTATUS GetClssLightStatus(void)
{
    return sg_eClssLightStatus;
}

void PiccLightOpen(void)
{
    sg_fd_blueLED = ex_open("/sys/devices/platform/misc/rf_blue/power", O_WRONLY, S_IRWXU|S_IRWXG|S_IRWXO);
    sg_fd_yellowLED = ex_open("/sys/devices/platform/misc/rf_yellow/power", O_WRONLY, S_IRWXU|S_IRWXG|S_IRWXO);
    sg_fd_greenLED = ex_open("/sys/devices/platform/misc/rf_green/power", O_WRONLY, S_IRWXU|S_IRWXG|S_IRWXO);
    sg_fd_redLED = ex_open("/sys/devices/platform/misc/rf_red/power", O_WRONLY, S_IRWXU|S_IRWXG|S_IRWXO);
    sg_fd_logoLED = ex_open("/sys/devices/platform/misc/rf_logo/power", O_WRONLY, S_IRWXU|S_IRWXG|S_IRWXO);

    SetClssLightStatus(CLSSLIGHTSTATUS_INIT);
}

void PiccLightClose(void)
{
    write(sg_fd_blueLED, "0", 1);
    write(sg_fd_yellowLED, "0", 1);
    write(sg_fd_greenLED, "0", 1);
    write(sg_fd_redLED, "0", 1);
    write(sg_fd_logoLED, "0", 1);

    close(sg_fd_blueLED);
    close(sg_fd_yellowLED);
    close(sg_fd_greenLED);
    close(sg_fd_redLED);
    close(sg_fd_logoLED);
}

static void PiccLight(uchar ucLedIndex, uchar ucOnOff)
{
    int i=0;
    unsigned int iPicXPercent[4]={0};
    int iImageWidth=0, iImageHeight=0, iGapLen=0;

    Gui_GetImageSize(AddPrefixResPath("BlueOff.png"), &iImageWidth, &iImageHeight);
    for (i = 0; i < 4; i++)
    {
        iGapLen = (Gui_GetScrWidth() - 4 * iImageWidth) / 5;
        iPicXPercent[i] = (iGapLen * (i + 1) + iImageWidth * i) * 100 / Gui_GetScrWidth();
    }

    //leds control
    if ((ucLedIndex & PICC_LED_BLUE) && (ucOnOff == 1)) {
        Gui_DrawImage(AddPrefixResPath("BlueOn.png"), iPicXPercent[0], 2);
        if (sg_fd_blueLED > 0) {
            write(sg_fd_blueLED, "1", 1);
        }
    }
    else {
        Gui_DrawImage(AddPrefixResPath("BlueOff.png"), iPicXPercent[0], 2);
        if (sg_fd_blueLED > 0) {
            write(sg_fd_blueLED, "0", 1);
        }
    }


    if ((ucLedIndex & PICC_LED_YELLOW) && (ucOnOff == 1)) {
        Gui_DrawImage(AddPrefixResPath("YellowOn.png"), iPicXPercent[1], 2);
        if (sg_fd_yellowLED > 0) {
            write(sg_fd_yellowLED, "1", 1);
        }
    }
    else {
        Gui_DrawImage(AddPrefixResPath("YellowOff.png"), iPicXPercent[1], 2);
        if (sg_fd_yellowLED > 0) {
            write(sg_fd_yellowLED, "0", 1);
        }
    }

    if ((ucLedIndex & PICC_LED_GREEN) && (ucOnOff == 1)){
        Gui_DrawImage(AddPrefixResPath("GreenOn.png"), iPicXPercent[2], 2);
        if (sg_fd_greenLED > 0) {
            write(sg_fd_greenLED, "1", 1);
        }
    }
    else {
        Gui_DrawImage(AddPrefixResPath("GreenOff.png"), iPicXPercent[2], 2);
        if (sg_fd_greenLED > 0) {
            write(sg_fd_greenLED, "0", 1);
        }
    }

    if ((ucLedIndex & PICC_LED_RED) && (ucOnOff == 1)) {
        Gui_DrawImage(AddPrefixResPath("RedOn.png"), iPicXPercent[3], 2);
        if (sg_fd_redLED > 0) {
            write(sg_fd_redLED, "1", 1);
        }
    }
    else {
        Gui_DrawImage(AddPrefixResPath("RedOff.png"), iPicXPercent[3], 2);
        if (sg_fd_redLED > 0) {
            write(sg_fd_redLED, "0", 1);
        }
    }

    if (ucLedIndex & PICC_LED_CLSS) {
        if (sg_fd_logoLED > 0) {
            if (ucOnOff == 1) {
                write(sg_fd_logoLED, "1", 1);
            }
            else {
                write(sg_fd_logoLED, "0", 1);
            }
        }
    }
}

//set callback function from GUI, when GUI is geting keys, when call this function after register.
int ClssVirtualLight(gui_callbacktype_t eType, void *pvData, int *piDataLen)
{
	CLSSLIGHTSTATUS eCurStatus=0;

	eCurStatus = GetClssLightStatus();
	//if status has not change, return without doing anything.
	if(sg_ePrevStatus == eCurStatus)
	{
		return GUI_OK;
	}

	switch (GetClssLightStatus())
	{
		case CLSSLIGHTSTATUS_NOTREADY:
			PiccLight(PICC_LED_ALL, 0);
			break;
		case CLSSLIGHTSTATUS_IDLE:
			PiccLight(PICC_LED_BLUE | PICC_LED_CLSS, 1);
			break;
		case CLSSLIGHTSTATUS_READYFORTXN:
			PiccLight(PICC_LED_BLUE | PICC_LED_CLSS, 1);
			break;
		case CLSSLIGHTSTATUS_PROCESSING:
			PiccLight(PICC_LED_BLUE | PICC_LED_YELLOW | PICC_LED_CLSS, 1);
			break;
		case CLSSLIGHTSTATUS_READCARDDONE:
		case CLSSLIGHTSTATUS_REMOVECARD:
		case CLSSLIGHTSTATUS_DIALING:
		case CLSSLIGHTSTATUS_SENDING:
		case CLSSLIGHTSTATUS_RECEIVING1:
		case CLSSLIGHTSTATUS_RECEIVING2:
		case CLSSLIGHTSTATUS_PRINTING:
			PiccLight(PICC_LED_BLUE | PICC_LED_YELLOW | PICC_LED_GREEN | PICC_LED_CLSS, 1);
			break;
		case CLSSLIGHTSTATUS_COMPLETE:
			PiccLight(PICC_LED_BLUE | PICC_LED_YELLOW | PICC_LED_GREEN | PICC_LED_CLSS, 1);
			break;
		case CLSSLIGHTSTATUS_ERROR:
			PiccLight(PICC_LED_RED | PICC_LED_CLSS, 1);
			break;
		default:
			break;
	}

	sg_ePrevStatus = eCurStatus;
	return GUI_OK;
}
//******************************************************************************************************
// Modified by Kim_LinHB 2014-6-8 v1.01.0000
//modified by kevinliu 2015/11/27
int TransClssSale(void)
{
	int	 iRet = 0, iLen = 0, i = 0;
	uchar ucTemp[100] = {0}, ucTranAct = 0, ucKernType = 0, ucAcType = 0, ucMSDPath = 0;
	uchar ucPathType = 0;
	uchar ucTempPAN[24]={0};

	TransInit(SALE);
	ClssTransInit();
	vAppInitSchemeId();

	//get amount
	iRet = GetAmount();
	if(iRet)
	{
		return iRet; //EPT-17;
	}
	//prepare transaction parameter
	iRet = ClssPreProcTxnParam();
	if(iRet)
	{
		return ERR_TRAN_FAIL;
	}

	while(1)
	{
		//contactless transaction flow
		iRet = ClssProcFlow_ALL();
		if (iRet)
		{
			if (iRet == App_Try_Again)
			{
				DelayMs(1200);
				continue;
			}
			else if(iRet != ERR_USERCANCEL && iRet != ERR_TIMEOUT){ //EPT-17
			    ProcError_Picc(iRet);
			    iRet = ERR_TRAN_FAIL;
			}
			PiccClose();
			return iRet;
		}

		ucAcType = ucAppGetTransACType();
		//AC TYPE
        if ((iRet == 0) && (ucAppGetTransACType() == AC_AAC)
                && ((ucAppGetAppType() == KERNTYPE_VIS) || (ucAppGetAppType() == KERNTYPE_AE)))
        {
            iRet = EMV_DENIAL;
        }
		//create output
		iRet = AppConv_CreateOutCome(iRet, ucAcType, &g_tOutComeData);
		if (iRet)
		{
			if (iRet == App_Try_Again)
			{
				DelayMs(1200);
				continue;
			}
			ProcError_Picc(iRet);
			PiccClose();
			return ERR_TRAN_FAIL;
		}
		else
		{
			break;
		}
	}

	ucKernType = ucAppGetAppType();
	ucPathType = ucAppGetTransPath();

	//FOR TEST
//	Gui_ClearScr();
//	Gui_ShowMsgBox(GetCurrTitle(), gl_stTitleAttr, _T("CHECK RESULT"), gl_stCenterAttr, GUI_BUTTON_CANCEL, 10, NULL);

	//get track data
	memset(glProcInfo.szTrack1, 0, sizeof(glProcInfo.szTrack1));
	memset(glProcInfo.szTrack2, 0, sizeof(glProcInfo.szTrack2));
	//process data
	if(ucKernType == KERNTYPE_VIS)
	{
		//get MSD and Wave2 track 1 data(ASCII)
		Clss_nGetTrack1MapData_Wave (glProcInfo.szTrack1, &iLen);

		if(ucPathType == CLSS_VISA_MSD)
		{
			Clss_GetMSDType_Wave(&ucMSDPath);
			//get MSD track 2 data
			Clss_nGetTrack2MapData_Wave (glProcInfo.szTrack2, &iLen);
		}
		//chip or MSD without trk2map data
		if(strlen((char *)glProcInfo.szTrack2) == 0)
		{
			//get track 2 data from ICC
			iLen = 0;
			memset(ucTemp, 0, sizeof(ucTemp));
			iRet = Clss_GetTLVData(0x57, ucTemp, &iLen, ucKernType);
			if(iRet == EMV_OK)
			{
				memset(glProcInfo.szTrack2, 0, sizeof(glProcInfo.szTrack2));
				PubBcd2Asc0(ucTemp, iLen, glProcInfo.szTrack2);
				iLen = iLen*2;
				glProcInfo.szTrack2[iLen] = '\0';
			}
		}
	}
	else if(ucKernType == KERNTYPE_MC)
	{
		iLen = 0;
		memset(ucTemp, 0, sizeof(ucTemp));
		//get track 1 data only for paypass
		iRet = Clss_GetTLVData(0x56 , glProcInfo.szTrack1, &iLen, ucKernType);

		//get track 2 data for paypass
		iLen = 0;
		memset(ucTemp, 0, sizeof(ucTemp));
		if (ucPathType == CLSS_MC_MAG)
		{
			iRet = Clss_GetTLVData(0x9F6B, ucTemp, &iLen, ucKernType);
		}
		else if (ucPathType == CLSS_MC_MCHIP)
		{
			iRet = Clss_GetTLVData(0x57, ucTemp, &iLen, ucKernType);
		}
		if(iRet == EMV_OK)
		{
			memset(glProcInfo.szTrack2, 0, sizeof(glProcInfo.szTrack2));
			PubBcd2Asc0(ucTemp, iLen, glProcInfo.szTrack2);
			iLen = iLen*2;
			glProcInfo.szTrack2[iLen] = '\0';
		}
	}
	else if(ucKernType == KERNTYPE_AE)
	{
		if(ucPathType == AE_MAGMODE)
		{
			memset(ucTemp, 0, sizeof(ucTemp));
			Clss_nGetTrackMapData_AE(0x01, glProcInfo.szTrack1, (uchar*)&iLen);
			Clss_nGetTrackMapData_AE(0x02, ucTemp, (uchar*)&iLen);
			memcpy(glProcInfo.szTrack2, ucTemp + 1, iLen - 1);
		}
		//chip or MSD without trk2map data
		if(strlen((char *)glProcInfo.szTrack2) == 0)
		{
			//tag = 0x57;
			iLen = 0;
			memset(ucTemp, 0, sizeof(ucTemp));
			iRet = Clss_GetTLVData(0x57, ucTemp, &iLen, ucKernType);
			if(iRet == EMV_OK)
			{
				memset(glProcInfo.szTrack2, 0, sizeof(glProcInfo.szTrack2));
				PubBcd2Asc0(ucTemp, iLen, glProcInfo.szTrack2);
				iLen = iLen*2;
				glProcInfo.szTrack2[iLen] = '\0';
			}
		}
	}
	else if(ucKernType == KERNTYPE_JCB)
	{
		iLen = 0;
		memset(ucTemp, 0, sizeof(ucTemp));
		//get track 1 data only for paypass
		iRet = Clss_GetTLVData(0x56 , glProcInfo.szTrack1, &iLen, ucKernType);

		//get track 2 data for paypass
		iLen = 0;
		memset(ucTemp, 0, sizeof(ucTemp));
		if ((ucPathType == CLSS_JCB_EMV) || (ucPathType == CLSS_JCB_LEGACY))
		{
			iRet = Clss_GetTLVData(0x57, ucTemp, &iLen, ucKernType);
		}
		//CLSS_JCB_MAG
		else if(ucPathType == CLSS_JCB_MAG)
		{
			iRet = Clss_GetTLVData(0x9F6B, ucTemp, &iLen, ucKernType);
		}

		if(iRet == EMV_OK)
		{
			memset(glProcInfo.szTrack2, 0, sizeof(glProcInfo.szTrack2));
			PubBcd2Asc0(ucTemp, iLen, glProcInfo.szTrack2);
			iLen = iLen*2;
			glProcInfo.szTrack2[iLen] = '\0';
		}
	}
	else if(ucKernType == KERNTYPE_ZIP)
	{
		if((ucPathType == CLSS_DPAS_MAG) || (ucPathType == CLSS_DPAS_ZIP))
		{
			Clss_GetTrackMapData_DPAS(0x01, glProcInfo.szTrack1, &iLen);
			Clss_GetTrackMapData_DPAS(0x02, glProcInfo.szTrack2, &iLen);
		}
		//chip or MSD without trk2map data
		if(strlen((char *)glProcInfo.szTrack2) == 0)
		{
			//tag = 0x57;
			iLen = 0;
			memset(ucTemp, 0, sizeof(ucTemp));
			iRet = Clss_GetTLVData(0x57, ucTemp, &iLen, ucKernType);
			if(iRet == EMV_OK)
			{
				memset(glProcInfo.szTrack2, 0, sizeof(glProcInfo.szTrack2));
				PubBcd2Asc0(ucTemp, iLen, glProcInfo.szTrack2);
				iLen = iLen*2;
				glProcInfo.szTrack2[iLen] = '\0';
			}
		}
	}

	for(i=0;i<iLen;i++)
	{
		if(glProcInfo.szTrack2[i] == 'D')
		{
			glProcInfo.szTrack2[i] = '=';
			break;
		}
	}

	// get PAN from track 2 (PAN)
	iRet = GetPanFromTrack2(glProcInfo.stTranLog.szPan, glProcInfo.stTranLog.szExpDate, iLen);
	if( iRet!=0 )
	{
		DispMagReadErr();
		PiccClose();
		return ERR_TRAN_FAIL;
	}
	
	//added by kevin liu 20160628 bug 834
	//get PAN compare with PAN in track2
	iLen = 0;
	memset(ucTemp, 0, sizeof(ucTemp));
	//Application Primary Account Number (PAN) Sequence Number
	iRet = Clss_GetTLVData(0x5A, ucTemp, &iLen, ucKernType);
	if(iRet == EMV_OK)
	{
		memset(ucTempPAN, 0, sizeof(ucTempPAN));
		PubBcd2Asc(ucTemp, iLen, ucTempPAN);
		PubTrimTailChars(ucTempPAN, 'F');

		if(0 != strcmp((char *)glProcInfo.stTranLog.szPan, (char *)ucTempPAN)) {
			DispMagReadErr();
			PiccLight(PICC_LED_ALL, 0);
			PiccClose();
			return ERR_TRAN_FAIL;
		}
	}

	//get issuer and acquirer from PAN
	iRet = MatchCardTable(glProcInfo.stTranLog.szPan);
	if( iRet!=0 )
	{
		Gui_ClearScr();
		PubBeepErr();
		Gui_ShowMsgBox(GetCurrTitle(), gl_stTitleAttr, _ICON_FAIL, _T("UNSUPPORTED\nCARD"), gl_stCenterAttr, GUI_BUTTON_CANCEL, 2, NULL);
		PiccClose();
		return ERR_TRAN_FAIL;
	}

	iLen = 0;
	memset(ucTemp, 0, sizeof(ucTemp));
	//Application Primary Account Number (PAN) Sequence Number
	iRet = Clss_GetTLVData(0x5f34, ucTemp, &iLen, ucKernType);
	if(iRet == EMV_OK)
	{
		glProcInfo.stTranLog.ucPanSeqNo = ucTemp[0];
		glProcInfo.stTranLog.bPanSeqOK = TRUE;
	}

	iLen = 0;
	memset(ucTemp, 0, sizeof(ucTemp));
	iRet = Clss_GetTLVData(0x50, ucTemp, &iLen, ucKernType);
	if(iRet == EMV_OK)
	{
		memcpy(glProcInfo.stTranLog.szAppLabel, ucTemp, iLen);
		glProcInfo.stTranLog.szAppLabel[iLen]=0;
	}

	iLen = 0;
	memset(ucTemp, 0, sizeof(ucTemp));
	iRet = Clss_GetTLVData(0x5f24, ucTemp, &iLen, ucKernType);
	if(iRet == EMV_OK)
	{
		PubBcd2Asc(ucTemp, 2, glProcInfo.stTranLog.szExpDate);
		glProcInfo.stTranLog.szExpDate[4]=0;
	}

	iLen = 0;
	memset(ucTemp, 0, sizeof(ucTemp));
	iRet = Clss_GetTLVData(0x95, ucTemp, &iLen, ucKernType);
	if(iRet == EMV_OK)
	{
		memcpy(glProcInfo.stTranLog.sTVR, ucTemp, iLen);
	}

	iLen = 0;
	memset(ucTemp, 0, sizeof(ucTemp));
	iRet = Clss_GetTLVData(0x9b, ucTemp, &iLen, ucKernType);
	if(iRet == EMV_OK)
	{
		memcpy(glProcInfo.stTranLog.sTSI, ucTemp, iLen);
	}

	iLen = 0;
	memset(ucTemp, 0, sizeof(ucTemp));
	iRet = Clss_GetTLVData(0x9f36, ucTemp, &iLen, ucKernType);
	if(iRet == EMV_OK)
	{
		memcpy(glProcInfo.stTranLog.sATC,ucTemp,iLen);
	}

	iLen = 0;
	memset(ucTemp, 0, sizeof(ucTemp));
	iRet = Clss_GetTLVData(0x9f26, ucTemp, &iLen, ucKernType);
	if(iRet == EMV_OK)
	{
		memcpy(glProcInfo.stTranLog.sAppCrypto,ucTemp,iLen);  //szAppPreferName
	}
	iLen = 0;
	memset(ucTemp, 0, sizeof(ucTemp));
	iRet = Clss_GetTLVData(0x9f26, ucTemp, &iLen, ucKernType);
	if(iRet == EMV_OK)
	{
		memcpy(glProcInfo.stTranLog.szAppPreferName,ucTemp,iLen);
	}

//CVM process 
	if(g_tOutComeData.ucCVMType == RD_CVM_ONLINE_PIN)
	{
		iRet = GetPIN(FALSE);
		if(iRet != EMV_OK)
		{
			PiccClose();
			return ERR_TRAN_FAIL;
		}
	}
	else if(g_tOutComeData.ucCVMType == RD_CVM_SIG)
	{
		DoE_Signature();
	}
	else
	{
		OsLog(LOG_INFO, "%s - %d, ucCVMType = %02x", __FUNCTION__, __LINE__, g_tOutComeData.ucCVMType);
		//no cvm
		//consumer device
	}

//continue transaction according to AC type
	if(g_tOutComeData.ucTransRet == CLSS_ONLINE_REQUEST)
	{
		memset(sg_aucAuthData, 0, sizeof(sg_aucAuthData));
		memset(sg_aucIssuerScript, 0, sizeof(sg_aucIssuerScript));
		sg_iAuthDataLen = 0;
		sg_iScriptLen = 0;

		iRet = Clss_transmit(ucKernType);
		if(iRet != EMV_OK)
		{
			Inter_DisplayMsg(MSG_DECLINED);
			PiccClose();
			return ERR_TRAN_FAIL;
		}
		//TODO second tap card
		iRet = Clss_CompleteTrans();
		//TODO
//		if(iRet != EMV_OK)
//		{
//			ProcError_Picc(iRet);
//			return ERR_TRAN_FAIL;
//		}

		if( ChkIfSaveLog() )
		{
			SaveTranLog(&glProcInfo.stTranLog);
		}
		if( ChkIfPrnReceipt() )
		{
			if( glProcInfo.stTranLog.ucTranType!=VOID )
			{
				GetNewInvoiceNo();
			}
		}
		SaveRevInfo(FALSE);


		if( glProcInfo.stTranLog.ucTranType!=RATE_BOC &&
			glProcInfo.stTranLog.ucTranType!=RATE_SCB &&
			glProcInfo.stTranLog.ucTranType!=LOAD_CARD_BIN )
		{
			if(ChkIfCiti())
			{
				OfflineSend(OFFSEND_TRAN);
			}
			else
			{
				OfflineSend(OFFSEND_TC | OFFSEND_TRAN);
			}
		}

		CommOnHook(FALSE);

		if(ChkIfPrnReceipt())
		{
			PrintReceipt(PRN_NORMAL);
		}
		DispResult(0);
	}
	else if(g_tOutComeData.ucTransRet == CLSS_APPROVE)
	{
		// save for upload
		iLen = 0;
		SetClSSDE55(TRUE, glProcInfo.stTranLog.sIccData, &iLen);
		glProcInfo.stTranLog.uiIccDataLen = (ushort)iLen;

		GetNewTraceNo();
		sprintf((char *)glProcInfo.stTranLog.szRspCode, "00");
		glProcInfo.stTranLog.ulInvoiceNo = glSysCtrl.ulInvoiceNo;
		sprintf((char *)glProcInfo.stTranLog.szAuthCode, "%06lu", glSysCtrl.ulSTAN);

		SetOffBase(OffBaseDisplay);

		ucTranAct = glTranConfig[glProcInfo.stTranLog.ucTranType].ucTranAct;

		if (ucTranAct & ACT_INC_TRACE)
		{
			GetNewTraceNo();
		}
		//removed by Kevin Liu 20160614 bug828
//		DoE_Signature();

		if( ucTranAct & WRT_RECORD )
		{
			glProcInfo.stTranLog.uiStatus |= TS_NOSEND;
			SaveTranLog(&glProcInfo.stTranLog);
		}

		if( ucTranAct & PRN_RECEIPT )	// print slip
		{
			CommOnHook(FALSE);
			GetNewInvoiceNo();
			PrintReceipt(PRN_NORMAL);
		}
		DispResult(0);
	}
	else
	{
		Inter_DisplayMsg(MSG_DECLINED);
		iRet = ERR_NO_DISP;
	}
	PiccClose();
	return iRet;
}

int Clss_CompleteTrans(void)
{
	int iRet = 0;
	unsigned char ucKernelType = 0, aucRspCode[3] = {0}, ucOnlineResult = 0;
	unsigned char aucAuthCode[7] = {0};

	if((sg_iAuthDataLen == 0) || (sg_iScriptLen == 0))
	{
		return EMV_OK;
	}

	ucKernelType = ucAppGetAppType();
	memcpy(aucRspCode, glProcInfo.stTranLog.szRspCode, 2);
	memcpy(aucAuthCode, glProcInfo.stTranLog.szAuthCode, 6);

	if (glProcInfo.ucOnlineStatus == ST_ONLINE_FAIL)
		ucOnlineResult = ONLINE_FAILED;
	else if (glProcInfo.ucOnlineStatus == ST_ONLINE_APPV)
		ucOnlineResult = ONLINE_APPROVE;
	else if (!memcmp(aucRspCode, "89", 2))
		ucOnlineResult = ONLINE_ABORT;
	else
		ucOnlineResult = ONLINE_DENIAL;

	if ( ucKernelType != KERNTYPE_AE && ucKernelType != KERNTYPE_VIS &&
		ucKernelType != KERNTYPE_ZIP && ucKernelType != KERNTYPE_JCB )
	{
		return EMV_OK;
	}

	SetClssLightStatus(CLSSLIGHTSTATUS_READYFORTXN);
	Gui_ClearScr();
	Gui_ShowMsgBox(GetCurrTitle(), gl_stTitleAttr, NULL, _T("SECOND TAP CARD"), gl_stCenterAttr, GUI_BUTTON_NONE, 0,NULL);

	iRet = ClssDetectTapCard();
	if(iRet)
	{
		return iRet;
	}

	SetClssLightStatus(CLSSLIGHTSTATUS_PROCESSING);
	DispProcess();

	SetClssLightStatus(CLSSLIGHTSTATUS_REMOVECARD);
	if(ucKernelType == KERNTYPE_AE)
	{
		iRet = ClssCompleteTrans_AE(ucOnlineResult, aucRspCode, aucAuthCode, sg_aucAuthData, sg_iAuthDataLen, sg_aucIssuerScript, sg_iScriptLen);
	}
	else if(ucKernelType == KERNTYPE_VIS)
	{
		iRet = ClssCompleteTrans_WAVE(ucOnlineResult, aucRspCode, aucAuthCode, sg_aucAuthData, sg_iAuthDataLen, sg_aucIssuerScript, sg_iScriptLen);
	}
	else if(ucKernelType == KERNTYPE_ZIP)
	{
		iRet = ClssCompleteTrans_DPAS(ucOnlineResult, aucRspCode, aucAuthCode, sg_aucAuthData, sg_iAuthDataLen, sg_aucIssuerScript, sg_iScriptLen);
	}
	else if(ucKernelType == KERNTYPE_JCB)
	{
		iRet = ClssCompleteTrans_JCB(ucOnlineResult, aucRspCode, aucAuthCode, sg_aucAuthData, sg_iAuthDataLen, sg_aucIssuerScript, sg_iScriptLen);
	}

	if (iRet)
	{
		Inter_DisplayMsg(MSG_DECLINED);
		return iRet;
	}
	Inter_DisplayMsg(MSG_APPROVED);
	return EMV_OK;
}

int ClssCompleteTrans_JCB(uchar ucInOnlineResult, uchar aucRspCode[], uchar aucAuthCode[], uchar aucIAuthData[], int nIAuthDataLen,  uchar aucScript[], int nScriptLen)
{
	uchar ucCardType=0;
	int nRet=0;

   if (aucRspCode[0])
    {
	   nSetDETData((uchar*)"\x8A", 1, aucRspCode, 2);
    }
    if (nIAuthDataLen)
    {
    	nSetDETData((uchar*)"\x89", 1, aucAuthCode, nIAuthDataLen);
    }

    if (ucInOnlineResult == ONLINE_FAILED)
    {
    	nSetDETData((uchar*)"\x8A", 1, (uchar *)"Z1", 2);
        return CLSS_DECLINE;
    }

	if (nIAuthDataLen == 0 && nScriptLen == 0)
	{
		return EMV_NO_DATA;
	}

	ucCardType = ucAppGetTransPath();

	if (ucCardType == CLSS_JCB_EMV)
	{
        nRet = Clss_IssuerUpdateProc_JCB(aucScript, &nScriptLen);
        if(nRet)
        {
            appRemovePicc();
            return nRet;
        }
	}
	appRemovePicc();
	return EMV_OK;
}

int ClssCompleteTrans_DPAS(uchar ucInOnlineResult, uchar aucRspCode[], uchar aucAuthCode[], uchar aucIAuthData[], int nIAuthDataLen,  uchar aucScript[], int nScriptLen)
{
	uchar ucCardType=0;
	int nRet=0;

   if (aucRspCode[0])
    {
	   nSetDETData((uchar*)"\x8A", 1, aucRspCode, 2);
    }
    if (nIAuthDataLen)
    {
    	nSetDETData((uchar*)"\x89", 1, aucAuthCode, nIAuthDataLen);
    }

    if (ucInOnlineResult == ONLINE_FAILED)
    {
    	nSetDETData((uchar*)"\x8A", 1, (uchar *)"Z1", 2);
        return CLSS_DECLINE;
    }

	if (nIAuthDataLen == 0 && nScriptLen == 0)
	{
		return EMV_NO_DATA;
	}

	ucCardType = ucAppGetTransPath();

	if (ucCardType == CLSS_DPAS_EMV)
	{
        nRet = Clss_IssuerUpdateProc_DPAS(ucInOnlineResult, aucScript, &nScriptLen);
        if(nRet)
        {
            appRemovePicc();
            return nRet;
        }
	}
	appRemovePicc();
	return EMV_OK;
}

int ClssCompleteTrans_AE(uchar ucInOnlineResult, uchar aucRspCode[], uchar aucAuthCode[], uchar aucIAuthData[], int nIAuthDataLen,  uchar aucScript[], int nScriptLen)
{
	unsigned char ucOnlineMode=0;
	ONLINE_PARAM ptOnlineParam;
	unsigned char pucAdviceFlag=0;
	uchar aucTmTransCapa[4]={0};
	int iLen=0, nRet=0;

	if (nIAuthDataLen == 0 && nScriptLen == 0)
	{
		return EMV_NO_DATA;
	}

	ucOnlineMode = AE_PARTIALONLINE;
	Clss_GetTLVData_AE(0x9F6E, aucTmTransCapa, &iLen);
	if ((aucTmTransCapa[0] & 0x20) != 0)//full online supported
	{
		if (g_ucRemovalTimeup == 1)
		{
			ucOnlineMode = AE_PARTIALONLINE;
		}
		else
		{
			ucOnlineMode = AE_FULLONLINE;
		}
	}
	else//full online not supported
	{
		ucOnlineMode = AE_PARTIALONLINE;
	}

	memset(&ptOnlineParam, 0, sizeof(ONLINE_PARAM));
	memcpy(ptOnlineParam.aucRspCode, aucRspCode, 2);
	ptOnlineParam.nAuthCodeLen = strlen((char *)aucAuthCode);
	memcpy(ptOnlineParam.aucAuthCode, aucAuthCode, 6);
	memcpy(ptOnlineParam.aucIAuthData, aucIAuthData, nIAuthDataLen);
	ptOnlineParam.nIAuthDataLen = nIAuthDataLen;
	memcpy(ptOnlineParam.aucScript, aucScript, nScriptLen);
	ptOnlineParam.nIAuthDataLen = nScriptLen;

	nRet = Clss_CompleteTrans_AE(ucInOnlineResult,ucOnlineMode, &ptOnlineParam,&pucAdviceFlag);
	if(nRet)
	{
	    appRemovePicc();
	    return nRet;
	}
	appRemovePicc();
	return EMV_OK;
}

int ClssCompleteTrans_WAVE(uchar ucInOnlineResult, uchar aucRspCode[], uchar aucAuthCode[], uchar aucIAuthData[], int nIAuthDataLen,  uchar aucScript[], int nScriptLen)
{
	uchar aucBuff[256] = {0};
	uchar aucCTQ[32] = {0};
	int iCTQLen = 0, iLen = 0, iRet = 0;
	uchar ucKernType = 0;

	if (nIAuthDataLen == 0 && nScriptLen == 0)
	{
		return EMV_NO_DATA;
	}

	if (ucInOnlineResult != ONLINE_APPROVE)
		return CLSS_DECLINE;

	memset(aucCTQ, 0, sizeof(aucCTQ));
	if ((g_tClssPreProcInfoIn.aucReaderTTQ[2] & 0x80) == 0x80
		&& Clss_GetTLVData_Wave(0x9F6C, aucCTQ, &iCTQLen) == 0
		&& (aucCTQ[1] & 0x40) == 0x40)
	{
        memset(aucBuff, 0, sizeof(aucBuff));
        iRet = Clss_FinalSelect_Entry(&ucKernType, aucBuff, &iLen);
        if (iRet != 0)
        {
//				continue;
            appRemovePicc();
            return iRet;
        }

        iRet = Clss_IssuerAuth_Wave(aucIAuthData, nIAuthDataLen);
        if(iRet)
        {
            appRemovePicc();
            return iRet;
        }
        iRet = Clss_IssScriptProc_Wave(aucScript, nScriptLen);
        if(iRet)
        {
            appRemovePicc();
            return iRet;
        }
        appRemovePicc();
	}
	return EMV_OK;
}

int ClssTransInit(void)
{
	int i, iRet;
	EMV_APPLIST EMV_APP;

	Clss_CoreInit_Entry();

	Clss_DelAllAidList_Entry();
	Clss_DelAllPreProcInfo();

	memset(g_tCAPKeyList, 0, sizeof(g_tCAPKeyList));
	//read all CAPK, save into memory added by kevinliu
	for(i=0; i<glSysParam.uiCapkNum; i++)
	{
		iRet = ReadCAPKFile(i, &g_tCAPKeyList[i]);
	}

	for (i=0; i<glSysParam.uiAidNum; i++)
	{
		memset(&EMV_APP, 0, sizeof(EMV_APPLIST));
		iRet = ReadAIDFile(i, &EMV_APP);
//end modified by Kevinliu
		if(iRet != EMV_OK)
		{
			continue;
		}

		iRet = Clss_AddAidList_Entry(EMV_APP.AID, EMV_APP.AidLen, EMV_APP.SelFlag, KERNTYPE_DEF);
		if(iRet != EMV_OK) 
		{
			continue;
		}

		memset(&g_tClssPreProcInfoIn, 0, sizeof(Clss_PreProcInfo));
		g_tClssPreProcInfoIn.ulTermFLmt = 5000;	//Terminal Offline limit
		g_tClssPreProcInfoIn.ulRdClssTxnLmt = 100000;	//Reader contactless transaction limit
		g_tClssPreProcInfoIn.ulRdCVMLmt = 3000;	//Reader CVM limit
		g_tClssPreProcInfoIn.ulRdClssFLmt = 5000;	//Reader contactless Offline limit

		memcpy(g_tClssPreProcInfoIn.aucAID, EMV_APP.AID, EMV_APP.AidLen);
		g_tClssPreProcInfoIn.ucAidLen = EMV_APP.AidLen;

		g_tClssPreProcInfoIn.ucKernType = KERNTYPE_DEF;

		g_tClssPreProcInfoIn.ucCrypto17Flg = 1;
		g_tClssPreProcInfoIn.ucZeroAmtNoAllowed = 0;
		g_tClssPreProcInfoIn.ucStatusCheckFlg = 0;
		memcpy(g_tClssPreProcInfoIn.aucReaderTTQ, "\x36\x00\x80\x00", 4);	//Terminal Transaction Qualifiers

		g_tClssPreProcInfoIn.ucTermFLmtFlg = 1;
		g_tClssPreProcInfoIn.ucRdClssTxnLmtFlg = 1;
		g_tClssPreProcInfoIn.ucRdCVMLmtFlg = 1;
		g_tClssPreProcInfoIn.ucRdClssFLmtFlg=1;
		Clss_SetPreProcInfo_Entry(&g_tClssPreProcInfoIn);
	}
	return 0;
}

int nSetDETData(uchar *pucTag, uchar ucTagLen, uchar *pucData, uchar ucDataLen)
{
	int iRet = 0;
	uchar aucBuff[256] = {0},ucBuffLen = 0;

	if(pucTag == NULL || pucData == NULL)
	{
		return CLSS_PARAM_ERR;
	}
	memset(aucBuff, 0, sizeof(aucBuff));
	memcpy(aucBuff, pucTag, ucTagLen);
	ucBuffLen = ucTagLen;
	aucBuff[ucBuffLen++] = ucDataLen;
	memcpy(aucBuff+ucBuffLen, pucData, ucDataLen);
	ucBuffLen += ucDataLen;
	if(ucAppGetAppType() == KERNTYPE_MC)
	{
		iRet = Clss_SetTLVDataList_MC(aucBuff, ucBuffLen);
	}
	else if(ucAppGetAppType() == KERNTYPE_JCB)
	{
		iRet = Clss_SetTLVDataList_JCB(aucBuff, ucBuffLen);
	}
	else if(ucAppGetAppType() == KERNTYPE_ZIP)
	{
		iRet = Clss_SetTLVDataList_DPAS(aucBuff, ucBuffLen);
	}
	return iRet;
}

int Clss_GetTLVData(unsigned int uiTag, uchar *pucData, int *piDatalen, uchar ucFlag)
{
	int iRet = 0, iLen = 0;
	uchar ucTagList[3] = {0};
	uchar ucTagListLen = 0;
	uchar ucDataOut[100] = {0};
	uint uiActualDataOutLen = 0;

	if(ucFlag == KERNTYPE_VIS)
	{
		iRet = Clss_GetTLVData_Wave(uiTag, pucData, &iLen);
	}
	//TODO pucData size
	else if((ucFlag= KERNTYPE_MC) || (ucFlag == KERNTYPE_JCB) || (ucFlag == KERNTYPE_ZIP))
	{
		if(uiTag < 0xFF){
			ucTagListLen = 1;
		}else if((uiTag > 0xFF) && (uiTag < 0xFFFF)){
			ucTagListLen = 2;
		}else{
			ucTagListLen = 3;
		}
		memset(ucTagList, 0 ,sizeof(ucTagList));
		PubLong2Char(uiTag, ucTagListLen, ucTagList);
		if(ucFlag == KERNTYPE_MC) {
			iRet = Clss_GetTLVDataList_MC(ucTagList, ucTagListLen,
					sizeof(ucDataOut), ucDataOut, &uiActualDataOutLen);
		} else if(ucFlag == KERNTYPE_JCB) {
			iRet = Clss_GetTLVDataList_JCB(ucTagList, ucTagListLen,
					sizeof(ucDataOut), ucDataOut, &uiActualDataOutLen);
		} else if(ucFlag == KERNTYPE_ZIP) {
			iRet = Clss_GetTLVDataList_DPAS(ucTagList, ucTagListLen,
					sizeof(ucDataOut), ucDataOut, &uiActualDataOutLen);
		}
		if(RET_OK == iRet)
		{
			memcpy(pucData, ucDataOut, uiActualDataOutLen);
			iLen = uiActualDataOutLen;
		}
	}
	else if(ucFlag == KERNTYPE_AE)
	{
		iRet = Clss_GetTLVData_AE(uiTag,pucData,&iLen);
	}
	*piDatalen = iLen;

	return iRet;
}

int Clss_SetTLVData(unsigned int uiTag, uchar *pucData, int iDatalen, uchar ucFlag)
{
	int iRet = 0;
	uchar ucTagList[3] = {0};
	uchar ucTagListLen = 0;

	if(ucFlag == KERNTYPE_VIS)
	{
		iRet = Clss_SetTLVData_Wave(uiTag, pucData, iDatalen);
	}
	else if(ucFlag == KERNTYPE_AE)
	{
		iRet = Clss_SetTLVData_AE(uiTag, pucData, iDatalen);
	}
	else if((ucFlag == KERNTYPE_MC) || (ucFlag == KERNTYPE_JCB) || (ucFlag == KERNTYPE_ZIP))
	{
		if(uiTag < 0xFF) {
			ucTagListLen = 1;
		}else if((uiTag > 0xFF) && (uiTag < 0xFFFF)) {
			ucTagListLen = 2;
		}else{
			ucTagListLen = 3;
		}
		memset(ucTagList, 0 ,sizeof(ucTagList));
		PubLong2Char(uiTag, ucTagListLen, ucTagList);
		iRet = nSetDETData(ucTagList, ucTagListLen, pucData, iDatalen);
	}
	return iRet;
}


int Clss_transmit(uchar ucKerId)
{
	int	iRet = 0, iRetryPIN = 0;
	int iLen = 0;
	ulong	ulICCDataLen = 0;
	uchar	*psICCData = NULL, *psTemp = NULL;

	Clss_SetTLVData(0x9f27, "\x80",1, ucKerId);

	//处理交易返回
	SetCommReqField();	
	// prepare online DE55 data
	iRet = SetClSSDE55(FALSE, &glSendPack.sICCData[2], &iLen);
	if( iRet!=0 )
	{
		glProcInfo.ucOnlineStatus = ST_ONLINE_FAIL;
		return ONLINE_FAILED;
	}

	PubASSERT( iLen < LEN_ICC_DATA );
	PubLong2Char((ulong)iLen, 2, glSendPack.sICCData);
	memcpy(glProcInfo.stTranLog.sIccData, &glSendPack.sICCData[2], iLen);	// save for batch upload
	glProcInfo.stTranLog.uiIccDataLen = (ushort)iLen;
	
	// 判断上次交易是否需要进行冲正等
	iRet = TranReversal();
	if( iRet!=0 )
	{
		glProcInfo.ucOnlineStatus = ST_ONLINE_FAIL;
		return ONLINE_FAILED;
//		return iRet;
	}
	
	// Some banks may want TC to be sent prior to sale
	iRetryPIN = 0;
	while( 1 )
	{
		if (ChkIfAmex() || ChkCurAcqName("AMEX", FALSE))
		{
			GetNewInvoiceNo();
		}
		
		iRet = SendRecvPacket();
		if( iRet!=0 )
		{
			glProcInfo.ucOnlineStatus = ST_ONLINE_FAIL;
			break;
		}

		if( memcmp(glRecvPack.szRspCode, "55", 2)!=0 || ++iRetryPIN>3 || !ChkIfNeedPIN() )
		{
			break;
		}
		
		// 重新输入PIN
		iRet = GetPIN(GETPIN_RETRY);
		if( iRet!=0 )
		{
//			return ONLINE_DENIAL;
			return iRet;
		}
		sprintf((char *)glSendPack.szSTAN, "%06lu", glSysCtrl.ulSTAN);
		memcpy(&glSendPack.sPINData[0], "\x00\x08", 2);
		memcpy(&glSendPack.sPINData[2], glProcInfo.sPinBlock, 8);
	}

	glProcInfo.ucOnlineStatus = ST_ONLINE_APPV;
	
	if( memcmp(glRecvPack.szRspCode, "00", LEN_RSP_CODE) != 0 )
	{
		return ERR_TRAN_FAIL;
	}
	
	ulICCDataLen = PubChar2Long(glRecvPack.sICCData, 2);
//	PubChar2Long(glRecvPack.sICCData, 2, &ulICCDataLen);
	psICCData = &glRecvPack.sICCData[2];
	if (ulICCDataLen != 0)
	{
		IssScrCon();
	}

	if( (glProcInfo.ucOnlineStatus == ST_ONLINE_APPV) && (memcmp(glProcInfo.stTranLog.szRspCode, "00", 2)==0) && (iRet == 0))
	{
		// update for reversal(maybe have script result)
		SetClSSDE55(FALSE, &glSendPack.sICCData[2], &iLen);
		PubLong2Char((ulong)iLen, 2, glSendPack.sICCData);
		glProcInfo.stTranLog.uiIccDataLen = (ushort)iLen;
		SaveRevInfo(TRUE);	// update reversal information
	}
	//get Issuer Authentication Data and Issuer script
	for(psTemp=psICCData; psTemp<psICCData+ulICCDataLen; )
	{
		iRet = GetTLVItem(&psTemp, psICCData+ulICCDataLen-psTemp, SaveRspICCData, FALSE);
	}
	
	// 交易失败处理
	if (iRet)
	{
		SaveRevInfo(FALSE);
		DispResult(iRet);
		return ERR_NO_DISP;
	}	
	if (memcmp(glProcInfo.stTranLog.szRspCode, "00", 2)!=0 )
	{
		SaveRevInfo(FALSE);
		DispResult(ERR_HOST_REJ);
		return ERR_NO_DISP;
	}
	//removed by Kevin Liu 2016014 bug828
//	DoE_Signature();

	return 0;
}


int IssScrCon(void)
{
	uchar mode=0,outActype[256],key, *sScriptData;
	int time,len, iLen;
	int ret;
	//iss_scrstrc iss_scrs;
	APDU_SEND send_com;
	APDU_RESP recv_com;

	Gui_ClearScr();
	Gui_ShowMsgBox("SALE", gl_stTitleAttr, NULL, NULL, gl_stCenterAttr, GUI_BUTTON_NONE, 0, NULL);

	while(1)
	{
		if(PiccOpen() != 0)
		{
			return ERR_TRAN_FAIL;
		}

		TimerSet(3,500);
		kbflush();
		while(1)
		{
			time = TimerCheck(3);
			if(!time)
			{
				PiccClose();
				return ERR_TRAN_FAIL;
			}
			if(kbhit() != NOKEY)
			{
				key = getkey();
				PiccClose();
				if(key == KEYCANCEL) return ERR_TRAN_FAIL;
			}

			ret = PiccDetect(mode,NULL,NULL,NULL,NULL);
			if(ret == 0) break;
			if(ret == 1 ||ret==2) return ERR_TRAN_FAIL;
			if(ret == 3||ret==5||ret==6)
			{
				PiccClose();
				DelayMs(100);
				continue;
			}
			if(ret == 4)
			{
				PiccClose();
				return ERR_TRAN_FAIL;
			}
		}
		break;
	}
	

	memcpy(send_com.Command,"\x00\xA4\x04\x00",4);
	send_com.Lc = 14;
	memcpy(send_com.DataIn,"1PAY.SYS.DDF01",14);
	send_com.Le = 256;
	ret = PiccIsoCommand(0,&send_com,&recv_com);
	if(ret != 0) return ERR_TRAN_FAIL;
	if(recv_com.SWA != 0x90 ||recv_com.SWB != 0x00) return ERR_TRAN_FAIL;
	
	
	memcpy(send_com.Command,"\x00\xA4\x04\x00",4);
	memcpy(send_com.DataIn, glProcInfo.stTranLog.sAID, strlen(glProcInfo.stTranLog.sAID));
	send_com.Le = 256;
	ret = PiccIsoCommand(0,&send_com,&recv_com);
	if(ret != 0) return ERR_TRAN_FAIL;
	if(recv_com.SWA != 0x90 ||recv_com.SWB != 0x00) return ERR_TRAN_FAIL;
	
	iLen = PubChar2Long(glRecvPack.sICCData, 2);
	sScriptData = &glRecvPack.sICCData[2];
	if (ChkIfAmex())
	{
		sScriptData += 6;
	}

	memset(outActype, 0, sizeof(outActype));
	len = 0;
	ret = GetSpecTLVItem(sScriptData, iLen, 0x91, outActype, (ushort *)&len);
	if (ret==0)
	{
		ret = Clss_IssuerAuth_Wave (outActype, len);
	}
	memset(outActype, 0, sizeof(outActype));
	len = 0;
	ret = GetSpecTLVItem(sScriptData, iLen, 0x71, outActype, (ushort *)&len);
	if (ret==0)
	{
		ret = Clss_IssScriptProc_Wave (outActype, len);
	}
	memset(outActype, 0, sizeof(outActype));
	len = 0;
	ret = GetSpecTLVItem(sScriptData, iLen, 0x72, outActype, (ushort *)&len);
	if (ret==0)
	{
		ret = Clss_IssScriptProc_Wave (outActype, len);
	}
	if(ret!= EMV_OK) 
	{
		return ERR_TRAN_FAIL;
	}

	return EMV_OK;
}

int GetTLVItem(uchar **ppsTLVString, int iMaxLen, SaveTLVData pfSaveData, uchar bExpandAll)
{
	int			iRet;
	uchar		*psTag, *psSubTag;
	uint		uiTag, uiLenBytes;
	ulong		lTemp;

	// skip null tags
	for(psTag=*ppsTLVString; psTag<*ppsTLVString+iMaxLen; psTag++)
	{
		if( (*psTag!=TAG_NULL_1) && (*psTag!=TAG_NULL_2) )
		{
			break;
		}
	}
	if( psTag>=*ppsTLVString+iMaxLen )
	{
		*ppsTLVString = psTag;
		return 0;	// no tag available
	}

	// process tag bytes
	uiTag = *psTag++;
	if( (uiTag & TAGMASK_FIRSTBYTE)==TAGMASK_FIRSTBYTE )
	{	// have another byte
		uiTag = (uiTag<<8) + *psTag++;
	}
	if( psTag>=*ppsTLVString+iMaxLen )
	{
		return -1;
	}

	// process length bytes
	if( (*psTag & LENMASK_NEXTBYTE)==LENMASK_NEXTBYTE )
	{
		uiLenBytes = *psTag & LENMASK_LENBYTES;
		lTemp = PubChar2Long(psTag+1, uiLenBytes);
	}
	else
	{
		uiLenBytes = 0;
		lTemp      = *psTag & LENMASK_LENBYTES;
	}
	psTag += uiLenBytes+1;
	if( psTag+lTemp>*ppsTLVString+iMaxLen )
	{
		return -2;
	}
	*ppsTLVString = psTag+lTemp;	// advance pointer of TLV string

	// save data
	(*pfSaveData)(uiTag, psTag, (int)lTemp);
	if( !IsConstructedTag(uiTag) || !bExpandAll )
	{
		return 0;
	}

	// constructed data
	for(psSubTag=psTag; psSubTag<psTag+lTemp; )
	{
		iRet = GetTLVItem(&psSubTag, psTag+lTemp-psSubTag, pfSaveData, TRUE);
		if( iRet<0 )
		{
			return iRet;
		}
	}

	return 0;
}

int GetSpecTLVItem(uchar *psTLVString, int iMaxLen, uint uiSearchTag, uchar *psOutTLV, ushort *puiOutLen)
{
	uchar		*psTag, *psTagStr, szBuff[10];
	uint		uiTag, uiLenBytes;
	ulong		lTemp;
	
	// skip null tags
    for (psTag=psTLVString; psTag<psTLVString+iMaxLen; psTag++)
    {
        if ((*psTag!=0x00) && (*psTag!=0xFF))
        {
            break;
        }
    }
    if ( psTag>=psTLVString+iMaxLen )
    {
        return -1;	// no tag available
    }
    
    while (1)
    {
        psTagStr = psTag;
        // process tag bytes
        uiTag = *psTag++;
        if ((uiTag & 0x1F)==0x1F)
        {	// have another byte
            uiTag = (uiTag<<8) + *psTag++;
        }
        if (psTag>=psTLVString+iMaxLen)
        {
            return -2;	// specific tag not found
        }
        
        // process length bytes
        if ((*psTag & 0x80)==0x80)
        {
            uiLenBytes = *psTag & 0x7F;
			//atoi
			memset(szBuff, 0, sizeof(szBuff));
			memcpy(szBuff, psTag+1, uiLenBytes);
			lTemp = atoi((char *)szBuff);
            //PubChar2Long(psTag+1, uiLenBytes, &lTemp);
        }
        else
        {
            uiLenBytes = 0;
            lTemp      = *psTag & 0x7F;
        }
        psTag += uiLenBytes+1;
        if (psTag+lTemp>psTLVString+iMaxLen)
        {
            return -2;	// specific tag not found also
        }
        
        // Check if tag needed
        if (uiTag==uiSearchTag)
        {
            *puiOutLen = (ushort)(psTag-psTagStr+lTemp);
            memcpy(psOutTLV, psTagStr, *puiOutLen);
            return 0;
        }
        
        if (IsConstructedTag(uiTag))
        {
            if (GetSpecTLVItem(psTag, (int)lTemp, uiSearchTag, psOutTLV, puiOutLen)==0)
            {
                return 0;
            }
        }
        
        psTag += lTemp;	// advance pointer of TLV string
        if (psTag>=psTLVString+iMaxLen)
        {
            return -2;
        }
    }
    return 0;
}

int IsConstructedTag(uint uiTag)
{
	int		i;
	
	for(i=0; (uiTag&0xFF00) && i<2; i++)
	{
		uiTag >>= 8;
	}
	
	return ((uiTag & 0x20)==0x20);
}

//added by Gillian Chen 2015/9/25
int nAppFindMatchProID(unsigned char *pucProID, int ucProIDLen)
{
	EMV_APPLIST EMV_APP;
	int i, iRet;

	if ( pucProID == NULL)
	{
		return EMV_PARAM_ERR;
	}
	else
	{
		// modify v1.00.0018  [23/09/2015 chenyy]
		for (i=0; i<MAX_APP_NUM; i++)
		{
			memset(&EMV_APP, 0, sizeof(EMV_APPLIST));
			iRet = EMVGetApp(i, &EMV_APP);
			if(iRet != EMV_OK)
			{
				continue;
			}
			iRet = Clss_AddAidList_Entry(EMV_APP.AID, EMV_APP.AidLen, EMV_APP.SelFlag, KERNTYPE_DEF);
			if(iRet != EMV_OK)
			{
				continue;
			}
			memset(&g_tProgInfo, 0, sizeof(Clss_ProgramID));
		 	g_tProgInfo.ulTermFLmt = g_tClssPreProcInfoIn.ulTermFLmt;
		 	g_tProgInfo.ulRdClssTxnLmt = g_tClssPreProcInfoIn.ulRdClssTxnLmt;
		 	g_tProgInfo.ulRdCVMLmt = g_tClssPreProcInfoIn.ulRdCVMLmt;
		 	g_tProgInfo.ulRdClssFLmt =  g_tClssPreProcInfoIn.ulRdClssFLmt;
			memcpy(g_tProgInfo.aucProgramId, pucProID, 17);
			g_tProgInfo.ucPrgramIdLen = ucProIDLen;
		 	g_tProgInfo.ucAmtZeroNoAllowed = 0;
		 	g_tProgInfo.ucStatusCheckFlg = g_tClssPreProcInfoIn.ucStatusCheckFlg;
		 	g_tProgInfo.ucTermFLmtFlg = g_tClssPreProcInfoIn.ucTermFLmtFlg;
		 	g_tProgInfo.ucRdClssTxnLmtFlg = g_tClssPreProcInfoIn.ucRdClssTxnLmtFlg;
		 	g_tProgInfo.ucRdCVMLmtFlg = g_tClssPreProcInfoIn.ucRdCVMLmtFlg;
		 	g_tProgInfo.ucRdClssFLmtFlg = g_tClssPreProcInfoIn.ucRdClssFLmtFlg;
		}
		return EMV_OK;
	}
}


#else
int TransClssSale(void)
{
	return 0;
}
#endif



