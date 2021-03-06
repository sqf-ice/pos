/*
 * ============================================================================
 * COPYRIGHT
 *               PAX TECHNOLOGY, Inc. PROPRIETARY INFORMATION
 *   This software is supplied under the terms of a license agreement or
 *   nondisclosure agreement with PAX  Technology, Inc. and may not be copied
 *   or disclosed except in accordance with the terms in that agreement.
 *      Copyright (C) 2012-? PAX Technology, Inc. All rights reserved.
 * Description:
 * Revision History:
 * Date                  Author                 Action
 *  2012年9月11日    Sabrina Fung                     created
 * ============================================================================
 */
 
 
#ifndef SHARED_LIBRARY_H_pub
#define SHARED_LIBRARY_H_pub


#ifdef __cplusplus
extern "C"
{
#endif


/****************************************************************************


                            1.字符串类型转换



*****************************************************************************/

/****************************************************************************
功能： 	    将2进制源串分解成双倍长度可读的16进制串，BCD 码转换成可见ASCII码。
输入参数
psIn      源二进制串
iLen     需要转换的源串长度字节数
输出参数
psOut     转换后的ASC串
返回：  	无
****************************************************************************/
void MIDPubBcd2Asc(const char *psIn, int iLen, char *psOut);


/****************************************************************************
功能：  	将2进制串分解成双倍长度可读的16进制串,转换后的串以‘\0’结束。
                BCD码转换成ASCII码字符串，并带结束符
输入参数
psIn      源串
iLen  需要转换的源串长度字节数
输出参数
psOut    目标串
返回：  	无
****************************************************************************/
void MIDPubBcd2Asc0(const char *psIn, int iLen, char *pszOut);


/****************************************************************************
功能： 	    将可读的16进制串合并成其一半长度的二进制串,"12AB"-->0x12AB
输入参数
psIn      源串
iLen  源Ascii串长度
输出参数
psOut     目标串
返回：  	无
****************************************************************************/
void MIDPubAsc2Bcd(const char *psIn, int iLen, char *psOut);


/****************************************************************************
功能： 	    将数字串转变成长整数,相当于atoi(),但是该函数不要求零字符结束
输入参数
psString    数字串
iLen       需要转换的数字串的长度
输出参数
piOut      转换后的长整数
返回：  	无
****************************************************************************/
int MIDPubAsc2Int(const char *psString, int iLen, int *piOut);


/****************************************************************************
功能： 	    整型数转换成高位在前的字符串表示(按照网络字节格式存放)
输入参数
iSource   待转换的整数
iTargetLen    目标缓冲区的大小(字节数)
输出参数	psTarget   转换后的串
返回：  	无
****************************************************************************/
void MIDPubInt2Char(int iSource, int iTargetLen, char *psTarget);


/****************************************************************************
功能： 	    高位在前的字符串表示转换为整型数
输入参数
psSource   待转换的串，最多4个字符
iSrcLen     源串的长度，最大为4，超过时，取后psSource前的4个字符进行运算
输出参数
piTarget 转换后整数
返回：  	转换后整数
****************************************************************************/
int MIDPubChar2Int(const char *psSource, int iSrcLen, int *piTarget);


/****************************************************************************
功能： 	    将整数转换为BCD格式的串存放
输入参数
iSource    待转换的整数
iTargetLen          目标缓冲区的大小(字节数)
输出参数
psTarget    目标缓冲区
返回：  	无
****************************************************************************/
void MIDPubInt2Bcd(int iSource, int iTargetLen, char *psTarget);


/****************************************************************************
功能： 	    转换BCD串为整数
输入参数
psSource    待转换的BCD数据格式的串
iSrcLen      源串的待转换长度
输出参数
piTarget  转换后整数；
           piTarget地址可以为NULL，如果为NULL，则不输出
           atoi的返回是int，所以最大取值为2147483647
返回：  	转换后的整数，值同piTarget
****************************************************************************/
int MIDPubBcd2Int(const char *psSource, int iSrcLen, int *piTarget);



/****************************************************************************


                            2. 字符串格式化



*****************************************************************************/

/****************************************************************************
功能： 	        转换数字串到金额格式，如HKD$****.**, RMB****.**
输入参数
pszPrefix  前额的前缀，如“HKD 200.00”中的“HKD”或是“HKD$200.00”中的“HKD.”，
                             最大允许4个字符.如果为空，则不加前缀。
pszIn      需要转换的纯数字串,过滤到前面的‘0’，如”00100”会过滤为”100”,最大长度为40
cDeciPos  小数点后的金额位数，必需小于4
                            例如 当cDeciPos==2, "14277" 转换为 "142.77"
           if cDeciPos==0, "14277" 转换为 "14,277"或不变。
cMisc    其它参数
          GA_SEPARATOR: 使用逗号分隔大金额，如"12,345.67"
          GA_NEGATIVE: 金额前加负号： "SGD-123,45.67"
输出参数
pszOut      格式化后的金额串
返回：  	无
****************************************************************************/
#define GA_SEPARATOR	0x80
#define GA_NEGATIVE		0x40
void MIDPubConvAmount(const char *pszPrefix, const char *pszIn, char cDeciPos, char *pszOut, char cMisc);


/****************************************************************************
功能： 	    将字符串转换为大写
输入参数
pszStrInOut   待转换的字符串,以'\0'结束
输出参数
pszStrInOut   转换后的字符串,以'\0'结束
返回：  	无
****************************************************************************/
void MIDPubStrUpper(char *pszStrInOut);


/****************************************************************************
功能： 	        将字符串转换为小写
输入参数
pszStrInOut   待转换的字符串,以'\0'结束
输出参数
pszStrInOut   转换后的字符串,以'\0'结束
返回：  	无
****************************************************************************/
void MIDPubStrLower(char *pszStrInOut);


/****************************************************************************
功能： 	    删除字符串两端的空白字符(包括,空格/回车/换行/TAB字符)
            两端的或左侧右侧的概念:第一个和最后一个非此4个字符为标记,中间
            的不删除.
输入参数
pszStrInOut   待转换的字符串,以'\0'结束
输出参数
pszStrInOut   转换后的字符串,以'\0'结束
返回：  	无
****************************************************************************/
void MIDPubTrimStr(char *pszStrInOut);


/****************************************************************************
功能： 	    删除字符串两端的的特定字符,即删除左侧或右侧连续的特定字符
            两端的概念:同PubTrimStr.
输入参数
pszStrInOut   待转换的字符串,以'\0'结束
cSpcChar   特定字符
输出参数
pszStrInOut   转换后的字符串,以'\0'结束
返回：  	无
****************************************************************************/
void MIDPubTrimSpcStr(char *pszStrInOut, char cSpcChar);


/****************************************************************************
功能：          删除字符串左边连续的特定字符
输入参数
pszStringInOut     待转换的字符串,以'\0'结束
cRemoveChar  待删除字符
输出参数
pszString   转换后的字符串,以'\0'结束
返回：  	无

****************************************************************************/
void MIDPubTrimHeadChars(char *pszStrInOut, char cRemoveChar);


/****************************************************************************
功能： 	    删除字符串右边连续的特定字符
输入参数
pszStrInOut     待转换的字符串,以'\0'结束
cRemoveChar  待删除字符
输出参数
pszStrInOut    转换后的字符串,以'\0'结束
返回：  	无

****************************************************************************/
void MIDPubTrimTailChars(char *pszStrInOut, char cRemoveChar);


/****************************************************************************
功能： 	填充前导字符,以便达到iTargetLen长度
输入参数
pszStringInOut    待填充前导字符的串
iTargetLen       目标串长度
cAddChar         填充的字符
输出参数
pszStringInOut    填充后的串
返回：  	无
****************************************************************************/
void MIDPubAddHeadChars(char *pszStrInOut, int iTargetLen, char cAddChar);


/****************************************************************************
功能： 	        反转字符串
输入参数
pszStringInOut   待反转的字符串,以'\0'结束
输出参数
pszStringInOut   反转后的字符串,以'\0'结束
返回：  	无
****************************************************************************/
void MIDPubStrReverse(char *pszStringInOut);


/****************************************************************************
功能： 	    取字节的高四位
输入参数
cInChar    待取高四位的字节
输出参数	无
返回：  	高四位的字节
****************************************************************************/
char MIDPubHigh4Bit(const char cInChar);

/****************************************************************************
功能： 	    取字节的低四位
输入参数
cInChar    待取低四位的字节
输出参数	无
返回：  	低四位的字节
****************************************************************************/
char MIDPubLow4Bit(const char cInChar);


/****************************************************************************


                            3. 字符串间运算



*****************************************************************************/

/****************************************************************************
功能： 	    不区分大小写的字符串比较函数
输入参数
pszStr1    待比较的字符串1
pszStr2    待比较的字符串2
输出参数	无
返回：  	0          两串相等
        <0         pszStr1 < pszStr2
        >0         pszStr1 > pszStr2
                     参数错误，不考虑输出
****************************************************************************/
//int PubStrNoCaseCmp(const char *pszStr1, const char *pszStr2);


/****************************************************************************
功能： 	    将源串与目标串按一定长度异或后存入目标串,函数将把2个字符串前
           iLength字节异或后放入psOut。
输入参数
psVect1    被异或的串
psVect2    被异或的串
iLength   要异或的长度
输出参数
psOut      异或后的目标串
返回：  	无
说明：	   psVect1与psOut可为同一个地址,如果3个指针参数为NULL，则函数将直接返回；
***************************************************************************/
void MIDPubXor(const char *psVect1, const char *psVect2, int iLength, char *psOut);


/****************************************************************************
功能：  	两个ASC字符串相乘
输入参数
pszFaciend  被乘数ASC字符串，以\0为结束符
pszMultiplier乘数ASC字符串，以\0为结束符
输出参数
pszProduct相乘的结果，以\0为结束符
返回：  	无
****************************************************************************/
void MIDPubAscMul(const char *pszFaciend, const char *pszMultiplier, char *pszProduct);


/****************************************************************************
功能： 	    长度相等的两个ASC数字串相加。
输入参数
psAddend1   被加数ASC字符串，待运算串不足iLen要先手动做前补0操作，下同。
psAddend2   加数ASC字符串
iLen       字符串的长度
输出参数
pszSum      相加的结果
返回：  	无
****************************************************************************/
void MIDPubAscAdd(const char *psAddend1, const char *psAddend2, int iLen, char *pszSum);
#define PubSafeAscAdd(a,b,c)	PubAscAdd(a,b,c,a)


/****************************************************************************
功能： 	    长度相等的两个ASC字符串相减，前补0
输入参数
psMinuend    被减数ASC数字字符串
psSubtrahend 减数ASC数字字符串
iLen        字符串的长度
输出参数
pszResult    结果
返回：  	无
****************************************************************************/
void MIDPubAscSub(const char *psMinuend, const char *psSubtrahend, int iLen, char *pszResult);
#define PubSafeAscSub(a,b,c)	PubAscSub(a,b,c,a)


/****************************************************************************
功能： 	    ASC字符串减1,结果放在原串里
输入参数
psAscStrInOut    需减1的ASC字符串
iStrLen     psAscStr的长度
输出参数
psAscStrInOut    减1后的ASC字符串
返回：  	0        成功    successful
        1   失败    failed
****************************************************************************/
int MIDPubAscDec(char *psAscStrInOut, int iStrLen);


/****************************************************************************
功能： 	    ASC字符串加1,结果放在原串里
输入参数
psAscStrInOut    需加1的ASC字符串
iStrLen     psAscStr的长度
输出参数
psAscStrInOut    加1后的ASC字符串
返回：  	0   成功
        1   失败
****************************************************************************/
int MIDPubAscInc(char *psAscStrInOut, int iStrLen);


/****************************************************************************
功能： 	    长度相等的两个BCD字符串相加
输入参数
psAddend1   被加数BCD字符串
psAddend2   加数BCD字符串
iLen       字符串的长度
输出参数
psResult    相加的结果
返回     无
****************************************************************************/
void MIDPubBcdAdd(const char *psAddend1, const char *psAddend2, int iLen, char *psResult);
#define PubSafeBcdAdd(a,b,c)		PubBcdAdd(a, b, c, a)


/****************************************************************************
功能： 	    长度相等的两个BCD字符串相减，前补0
输入参数
psMinuend      被减数BCD字符串
psSubtrahend   减数BCD字符串
iLen          字符串的长度
输出参数
psResult        相减的结果
返回     无
****************************************************************************/
void MIDPubBcdSub(const char *psMinuend, const char *psSubtrahend, int iLen, char *psResult);
#define PubSafeBcdSub(a,b,c)		PubBcdSub(a, b, c, a)


/****************************************************************************
功能： 	    BCD字符串减1,结果放在原串里
输入参数
psBcdStrInOut    需减1的BCD字符串
iStrLen     psBcDStr的长度
输出参数
psBcdStrInOut    减1后的BCD字符串
返回：  	0       成功    successful
        1   失败    failed
****************************************************************************/
int MIDPubBcdDec(char *psBcdStrInOut, int iStrLen);


/****************************************************************************
功能： 	    BCD字符串加1,结果放在原串里
输入参数
psBcdStrInOut    需加1的BCD字符串
iStrLen     psBcdStr的长度
输出参数
psBcdStrInOut    加1后的BCD字符串
返回：  	0   成功    successful
        1   失败    failed
****************************************************************************/
int MIDPubBcdInc(char *psBcdStrInOut, int iStrLen);


/****************************************************************************


                                4. 标准算法



*****************************************************************************/


/****************************************************************************
功能： 	    计算LRC
输入参数
psData    需要计算的数据
iLength  数据的长度
输出参数
pcInit  LRC值
返回：  	无
****************************************************************************/
void MIDPubCalcLRC(const char *psData, int iLength, char *pcInit);

/****************************************************************************
功能： 	    校验卡号是否满足模10校验
输入参数
pszPan   卡号
输出参数	  无
返回：  	0    满足
        1    不满足
****************************************************************************/
int MIDPubChkCardNo(const char *pszPanNo);


/****************************************************************************


                                5. 时间函数



*****************************************************************************/
#define MID_STAR_YEAR 1980
#define MID_STAR_WEEK 6

/****************************************************************************
功能： 	    将一个int型整数转换为日期+时间字符串，以1980年1月1日0时开始算，
            最大时间可以到2116年1月1日
输入参数
iSecond            待转换的int整数,其单位的含义是秒
输出参数
pszYYYYMMDDhhmmss   得到的日期+时间字符串，14个字节以00h结尾
返回：  	None
说明：      1980年1月1日是默认的开始的日期，这个年份可自定义修改，
            修改START_YEAR，START_WEEK这两个宏定义即可
****************************************************************************/
void MIDPubInt2Time(int iSecond,char *pszYYYYMMDDhhmmss);


/****************************************************************************
功能： 	    将一个日期+时间字符串转换为int型整数，以1980年1月1日0时开始算，
            最大时间可以到2116年1月1日
输入参数
psYYYYMMDDhhmmss       需要被转换的日期+时间字符串
输出参数
None
返回：  	转换后int整数，其单位的含义是秒
****************************************************************************/
int MIDPubTime2Int(const char *psYYYYMMDDhhmmss);


/****************************************************************************
功能： 	    将一个int型整数转换为日期字符串，以1980年1月1日开始算
输入参数
iDay           待转换的int整数,其单位的含义是日
输出参数
pszYYYYMMDD     得到的日期字符串，8个字节以00h结尾
返回：  	None
说明：      1980年1月1日是默认的开始的日期，这个年份可自定义修改，方法为修改
            START_YEAR，START_WEEK这两个宏定义。
****************************************************************************/
void MIDPubInt2Day(int iDay, char *pszYYYYMMDD);


/****************************************************************************
功能： 	    将一个日期字符串转换为int型整数，以1980年1月1日开始算
输入参数
psYYYYMMDD       需要被转换的日期字符串
输出参数	None
返回：  	转换后int整数，其单位的含义是日
****************************************************************************/
int MIDPubDay2Int(const char *psYYYYMMDD);

/****************************************************************************
功能： 	     按照MASK定义的格式，检查日期时间格式是否有效
输入参数
psDateTime       被检查的日期时间字符串\
psMaskYYYYMMDDhhmmss
                 格式控制，该字符串可以为"YYYYMMDDhhmmss"或者其子串（必须是偶
                 数），详见用法中的例子。
                 如果年是YYYY格式，则必须为19YY或者20YY，否则认为非法
                 如果年是YY格式，则自动认为是20YY年
                 如果格式类似YMMD等非正常使用，则不保证判断结果正确。
输出参数
返回：  	 0，	有效
         1，2，3，4，5，6，分别代表年、月、日、时、分、秒、非法
         9，代表MASK格式错误
****************************************************************************/
int MIDPubIsValidTime(const char *psDateTime, const char *psMaskYYYYMMDDhhmmss);

/****************************************************************************
功能： 	     计算从输入时间开始,经过间隔时间后的时间点,有闰年处理
输入参数
psDateTime:      需要累加的基础时间,格式为YYYYMMDDhhmmss
pszDateTimeOut:  累加后的时间,格式为YYYYMMDDhhmmss
iInterval:      需要累加的时间,可为秒,分,时,天,月,年
psCalMode:       标记累加的单位,分别代表秒,分,时,天,月,年;
                                             可为“YY” “MM” “DD”“hh”“mm”“ss”，
                                            如果模式错误或者为NULL，则默认为“ss
输出参数	 None
返回：  	 None
****************************************************************************/
void MIDPubCalDateTime(const char *psDateTimeIn, char *pszDateTimeOut, int iInterval, char *psCalMode);


/****************************************************************************
功能： 	计算两个时间点之间的间隔
输入参数
psTimeBegin:    开始时间,格式为YYYYMMDDhhmmss
psTimeEnd:      结束时间,格式为YYYYMMDDhhmmss
psCalMode:      间隔的单位,分别代表秒,分,时,天,月,年；
                                           可为“YY”“MM”“DD”“hh”“mm”“ss”；
                                           如果模式错误或者为NULL，则默认为“ss”
输出参数
pcSignal:      相减的正负, 1:负,0:正
返回：  	间隔时间
****************************************************************************/
int MIDPubCalInterval(char *psTimeBegin, char *psTimeEnd, char *psCalMode, char *pcSignal);

/****************************************************************************
s

                               7. 获取信息



*****************************************************************************/
/****************************************************************************
功能： 	    获取错误信息
输入参数	None
输出参数	None
返回：  	错误信息字符串
****************************************************************************/
char* GetLastErrMsg(void);


/****************************************************************************
功能： 	    获取版本信息
输入参数	None
输出参数	None
返回：  	版本信息字符串
****************************************************************************/
char* PubGetVer(void);


void Con8(char *buf);
void ConMatrix(char *buf, int len);


#ifdef __cplusplus
};
#endif


#endif /* SHARED_LIBRARY_H_pub */

