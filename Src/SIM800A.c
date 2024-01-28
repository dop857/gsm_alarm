/******************************************************************************
����ͷ�ļ�
******************************************************************************/
#include "includes.h"
/******************************************************************************
���ݻ���������
******************************************************************************/
u8 GPRSLine = 0, GPRSRow = 0;//���յ��к��м���
u8 GPRSReceiveBuff[10][GPRS_RECEIVE_STRING_BUFFER_LENGTH];//GPRS�������ݻ�����
u8 GPRSSendBuff[100];//GPRS�������ݻ�����
/******************************************************************************
��������
******************************************************************************/
/*****************************************************************************/
// FUNCTION NAME: SIM800A_RebootIOInit
//
// DESCRIPTION:
//  sim900a�������ų�ʼ��
//
// CREATED: 2017-11-05 by zilin Wang
//
// FILE: SIM900A.c
/*****************************************************************************/
void SIM800A_RebootIOInit()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(SIM800A_Reboot_RCC_APBPERIPH_GPIOX, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = SIM800A_Reboot_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SIM800A_Reboot_GPIOX, &GPIO_InitStructure);
	
	GPIO_SetBits(SIM800A_Reboot_GPIOX, SIM800A_Reboot_GPIO_PIN);//����
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_CommandHandleDeinit
//
// DESCRIPTION:
//  ��ʼ���ṹ��
//
// CREATED: 2017-11-05 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
void SIM800A_CommandHandleDeinit(SIM800A_QueryTypeDef * CommandHandle, OS_EVENT * MsgQ)
{
    CommandHandle->DebugString = NULL;
    CommandHandle->SendString = NULL;
    CommandHandle->ReturnString = NULL;
    CommandHandle->ReturnStringExt1 = NULL;
    CommandHandle->ReturnStringExt2 = NULL;
    CommandHandle->GPRSUSART = USART_GPRS;
    CommandHandle->DEBUGUSART = USART_DEBUG;
    CommandHandle->MsgQ = MsgQ;
    CommandHandle->delayMs = 800;
    CommandHandle->retryTimes = 3;
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_CommandHandleInit
//
// DESCRIPTION:
//  ��ʼ�����ͽṹ��
//
// CREATED: 2017-10-28 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
void SIM800A_CommandHandleInit(SIM800A_QueryTypeDef * CommandHandle, USART_TypeDef * GPRSUSART, USART_TypeDef * DEBUGUSART, OS_EVENT * MsgQ)
{
    CommandHandle->DebugString = NULL;
    CommandHandle->SendString = NULL;
    CommandHandle->ReturnString = NULL;
    CommandHandle->ReturnStringExt1 = NULL;
    CommandHandle->ReturnStringExt2 = NULL;
    CommandHandle->GPRSUSART = GPRSUSART;
    CommandHandle->DEBUGUSART = DEBUGUSART;
    CommandHandle->MsgQ = MsgQ;
    CommandHandle->delayMs = 800;
    CommandHandle->retryTimes = 3;
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_SendSampleCommand
//
// DESCRIPTION:
//  ����һ��ֻ��һ������ֵ������
//
// CREATED: 2017-10-27 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
Status SIM800A_SendSampleCommand(SIM800A_QueryTypeDef * CommandHandle)
{ 
    u8 retryCnt = 0;
    u8 waitCnt = 0;
    u8 * receiveString;
    OS_Q_DATA ReceiveMsgQData;			//�����Ϣ����״̬�Ľṹ
    INT8U error;
    Status returnStatus;
    USARTSendString(CommandHandle->GPRSUSART, (u8*)CommandHandle->SendString);//���͵���Ϣ
    OSTimeDlyHMSM(0, 0, 1, CommandHandle->delayMs);
    while(1)
    {
        if (waitCnt < 5 && retryCnt <= CommandHandle->retryTimes)
        {
            
            OSQQuery(CommandHandle->MsgQ, &ReceiveMsgQData);
            if(ReceiveMsgQData.OSNMsgs > 0)
            {
                receiveString = OSQPend(CommandHandle->MsgQ, 0, &error);		//��Ϣ����
                if(strstr((char*)receiveString, CommandHandle->ReturnString) != NULL)		//�����Ϣƥ��
                {
                    ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);                       //�����Ϣ������
                    USARTSendString(CommandHandle->DEBUGUSART, (u8*)CommandHandle->DebugString);           //���ݲ�����ʾ���سɹ���Ϣ
                    returnStatus = ERROR_NONE;
                    break;
                }
                else
                {
                    //������Լ���ʶ�𷵻��ַ����Ĵ���
                    ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);  
                    returnStatus = ERROR_MISMACHING;
                }
            }
            waitCnt++;
            OSTimeDlyHMSM(0, 0, 0, CommandHandle->delayMs);
        }
        else if (retryCnt < CommandHandle->retryTimes)//�����ȴ�ʱ��
        {
            waitCnt = 0;//�ȴ�ʱ����0
            retryCnt++;
            USARTSendString(CommandHandle->GPRSUSART, (u8*)CommandHandle->SendString);//���·�����Ϣ
            CommandHandle->DebugString = "Command retrying\r\n";
            USARTSendString(CommandHandle->DEBUGUSART, (u8*)CommandHandle->DebugString);   
            OSTimeDlyHMSM(0, 0, 0, CommandHandle->delayMs);                   
        }
        else
        {
            CommandHandle->DebugString = "Failed\r\n";
            USARTSendString(CommandHandle->DEBUGUSART, (u8*)CommandHandle->DebugString);   
            returnStatus = ERROR_OVERTIME;
            break;
        }
    }
    return returnStatus;
} 
/*****************************************************************************/
// FUNCTION NAME: SIM800A_TryToHandshake
//
// DESCRIPTION:
//  ������������
//
// CREATED: 2017-10-29 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
Status SIM800A_TryToHandshake(SIM800A_QueryTypeDef * CommandHandle)
{
    CommandHandle->SendString = "AT\r\n";
    CommandHandle->ReturnString = "OK\r\n";
    CommandHandle->DebugString = "hand shaking\r\n";
    CommandHandle->delayMs = 500;
    CommandHandle->retryTimes = 8;
    return SIM800A_SendSampleCommand(CommandHandle);
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_Reboot
//
// DESCRIPTION:
//  --
//
// CREATED: 2017-11-05 by zilin Wang
//
// FILE: SIM900A_init.c
/*****************************************************************************/
void SIM800A_Reboot(void)
{
    GPIO_SetBits(GPIOB, GPIO_Pin_14);
    OSTimeDlyHMSM(0, 0, 2, 0);
    GPIO_ResetBits(GPIOB, GPIO_Pin_14);
}
/*****************************************************************************/
// FUNCTION NAME: NetWorkSendHTTPActionCommand
//
// DESCRIPTION:
//  ����HTTP��ȡ��ҳ���������������⣺����һ����������������ݣ��޷����볣���������
//
// CREATED: 2017-10-30 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
Status NetWorkSendHTTPActionCommand(SIM800A_QueryTypeDef * NetWorkCommandHandle, RequestMethod Method)
{ 
    if (Method == GET)//���ʹ��GET������
    {
        NetWorkCommandHandle->SendString = "AT+HTTPACTION=0\r\n";
        NetWorkCommandHandle->ReturnString = "OK\r\n";
        NetWorkCommandHandle->ReturnStringExt1 = "+HTTPACTION: 0,200,";//200�������ķ�����ҳ��sim800��sim900��һ���ո�
        NetWorkCommandHandle->DebugString = "send action success\r\n";
        Status str1Status = SIM800A_SendSampleCommand(NetWorkCommandHandle);
        Status returnStatus;
        if (ERROR_NONE != str1Status)
        {
            NetWorkCommandHandle->DebugString = "Send action failed";
            USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);   
            return str1Status;
        }
        OSTimeDlyHMSM(0, 0, 3, 0);
        u8 waitCnt = 0;
        u8 * receiveString;
        OS_Q_DATA ReceiveMsgQData;			//�����Ϣ����״̬�Ľṹ
        INT8U error;
        while(1)
        {
            if (waitCnt < 5)
            {
                OSQQuery(NetWorkCommandHandle->MsgQ, &ReceiveMsgQData);
                if(ReceiveMsgQData.OSNMsgs > 0)
                {
                    receiveString = OSQPend(NetWorkCommandHandle->MsgQ, 0, &error);		//��Ϣ����
                    if(strstr((char *)receiveString, NetWorkCommandHandle->ReturnStringExt1) != NULL)//���������Ϣ
                    {
                        ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);  //�����Ϣ������
                        NetWorkCommandHandle->DebugString = "HTTP get success";
                        USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);   
                        returnStatus = ERROR_NONE;
                        break;//����յ���ACTION��Ϣ��ֱ���˳�ѭ��
                    }
                    else
                    {
                        ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH); 
                        returnStatus = ERROR_MISMACHING;
                    }     
                }
                waitCnt++;//����ٵȴ�5s
                OSTimeDlyHMSM(0, 0, 1, NetWorkCommandHandle->delayMs);
            }
            else//���Դ�������
            {
                NetWorkCommandHandle->DebugString = "Failed:HTTP get failed"; //���ݲ�����ʾ������Ϣ
                USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);          
                returnStatus = ERROR_OVERTIME;
                break;
            }
        }
        return returnStatus;           
    }
    else if (Method == POST)//post�Ժ���д
    {
        return ERROR_WRONGARGUMENT;
    }
    else
    {
        return ERROR_WRONGARGUMENT;
    }
}



/*****************************************************************************/
// FUNCTION NAME: NetWorkHTTPReadCommand
//
// DESCRIPTION:
//  ��ȡhttp���ص��ַ���
//
// CREATED: 2017-10-31 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
Status NetWorkSendHTTPReadCommand(SIM800A_QueryTypeDef * NetWorkCommandHandle, u8 * returnString, int returnStringLength)
{
    //���÷����ַ���
    char SendString[25];
    sprintf(SendString, "AT+HTTPREAD=0,%d\r\n", returnStringLength);
    NetWorkCommandHandle->SendString = SendString;
    NetWorkCommandHandle->ReturnString = "+HTTPREAD:";//��������
    NetWorkCommandHandle->DebugString = "HTTP reading..\n";
    //��������
    SIM800A_SendSampleCommand(NetWorkCommandHandle);
    u8 waitCnt = 0;
    Status returnStatus;
    u8 error;
    OS_Q_DATA ReceiveMsgQData;			//�����Ϣ����״̬�Ľṹ
    u8 * receiveString;
    while(1)
    {
        if (waitCnt < 5)
        {
            OSTimeDlyHMSM(0, 0, 0, NetWorkCommandHandle->delayMs);
            OSQQuery(NetWorkCommandHandle->MsgQ, &ReceiveMsgQData);
            if(ReceiveMsgQData.OSNMsgs > 0)
            {
                receiveString = OSQPend(NetWorkCommandHandle->MsgQ, 0, &error);		//��Ϣ����
                for (u8 i = 0; i < returnStringLength; i++)
                {
                    *returnString = *receiveString;
                    returnString++;
                    receiveString++;
                }
                ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);  
                NetWorkCommandHandle->DebugString = "HTTP read success"; //���ݲ�����ʾ������Ϣ
                USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);        
                returnStatus = ERROR_NONE;
                break;
            }
            waitCnt++;
        }
        else//�ȴ�ʱ�����
        {
            NetWorkCommandHandle->DebugString = "Failed:HTTP read"; //���ݲ�����ʾ������Ϣ
            USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);  
            returnStatus = ERROR_OVERTIME;
            break;
        }
    }
    ClearStringQueue(NetWorkCommandHandle->MsgQ, GPRS_RECEIVE_STRING_BUFFER_LENGTH);//�����Ϣ����
    return returnStatus;
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_HTTPConnect
//
// DESCRIPTION:
//  ����һ��HTTP���Ӳ��Ͽ�
//
// CREATED: 2017-10-29 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
Status SIM800A_HTTPConnect(SIM800A_QueryTypeDef * NetWorkCommandHandle, u8 * requestURL, u8 * returnString, int returnStringLength)
{
    Status returnStatus = ERROR_NONE;
    u8 retryCnt = 0;
    while(retryCnt < NetWorkCommandHandle->retryTimes)
    {
        do//����κ�һ�������⣬ֱ������ѭ������������
        {
            NetWorkCommandHandle->ReturnString = "OK\r\n";
            NetWorkCommandHandle->SendString = "ATE0\r\n";   
            NetWorkCommandHandle->DebugString = "HTTP 0%\r\n";    
            SIM800A_SendSampleCommand(NetWorkCommandHandle);
            NetWorkCommandHandle->SendString = "AT+CFUN=1\r\n";   
            NetWorkCommandHandle->DebugString = "HTTP 10%\r\n"; 
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;
            NetWorkCommandHandle->SendString = "AT+SAPBR=1,1\r\n";   
            NetWorkCommandHandle->DebugString = "HTTP 15%\r\n"; 
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;
            NetWorkCommandHandle->SendString = "AT+HTTPINIT\r\n";   
            NetWorkCommandHandle->DebugString = "HTTP 20%\r\n"; 
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;
            NetWorkCommandHandle->SendString = "AT+HTTPPARA=\"CID\",\"1\"\r\n";   
            NetWorkCommandHandle->DebugString = "HTTP 30%\r\n"; 
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;
            //����URL��
            char FinalURLStr[140] = "AT+HTTPPARA=\"URL\",\"";
            char *str2 = "\"\r\n";
            if (ERROR_STACKOVERFLOW == MergeString(FinalURLStr, (char*)requestURL, 140))
            {
                USARTSendString(USART_DEBUG, "ERROR_STACKOVERFLOW when config url1\n");
            }
            if (ERROR_STACKOVERFLOW == MergeString(FinalURLStr, (char*)str2, 140))
            {
                USARTSendString(USART_DEBUG, "ERROR_STACKOVERFLOW when config url2\n");
            }
            NetWorkCommandHandle->SendString = FinalURLStr;   
            NetWorkCommandHandle->DebugString = "HTTP 40%\r\n"; 
            //USARTSendString(USART_DISPLAY, FinalURLStr);
            //������վurl
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;
            //ѡ����ʷ�����
            if (ERROR_NONE != (returnStatus = NetWorkSendHTTPActionCommand(NetWorkCommandHandle, GET)))
                break;        
            //����������ݣ�
            if (ERROR_NONE != (returnStatus = NetWorkSendHTTPReadCommand(NetWorkCommandHandle, returnString, returnStringLength)))
                break;           
        }
        while(0);
        //�������ӣ�
        NetWorkCommandHandle->SendString = "AT+HTTPTERM\r\n";   
        NetWorkCommandHandle->ReturnString = "OK\r\n";
        NetWorkCommandHandle->DebugString = "HTTP 95%\r\n"; 
        SIM800A_SendSampleCommand(NetWorkCommandHandle);
        NetWorkCommandHandle->SendString = "AT+SAPBR=0,1\r\n";  
        NetWorkCommandHandle->DebugString = "HTTP 100%\r\n"; 
        SIM800A_SendSampleCommand(NetWorkCommandHandle);
        if (returnStatus == ERROR_NONE)
            return ERROR_NONE;
        else
        {
            retryCnt++;
            USARTSendString(USART_DEBUG, "HTTP retrying\n");
        }
    }
    return returnStatus;
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_TCPSendPackage
//
// DESCRIPTION:
//  TCP������һ�����ݰ����������޷�������
//
// CREATED: 2017-11-02 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
Status SIM800A_TCPSendPackage(SIM800A_QueryTypeDef * NetWorkCommandHandle, u8 * data, int dataLength, u8 * IPString, int port)
{
    Status returnStatus = ERROR_NONE;
    u8 retryCnt = 0;    
    while(retryCnt < NetWorkCommandHandle->retryTimes)
    {
        do
        {
            NetWorkCommandHandle->SendString = "ATE0\r\n";
            NetWorkCommandHandle->ReturnString = "OK\r\n";
            NetWorkCommandHandle->DebugString = "TCP connecting 33%\r\n";
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;          
            NetWorkCommandHandle->SendString = "AT+CLPORT=\"TCP\",\"4000\"\r\n";
            NetWorkCommandHandle->DebugString = "TCP connecting 66%\r\n";
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;
            /******************************************************************************
            ����TCP���������ã�
            ******************************************************************************/
            char SendString[80] = "AT+CIPSTART=\"TCP\",\"";
            //��ǰ����з���url��ip��
            MergeString(SendString, (char*)IPString, 80);//��ʱ���ַ�������������AT+CIPSTART=\"TCP\",\"sys.czbtzn.cn
            //�ں����з���˿ں�
            char SendStringEnd[20];
            sprintf(SendStringEnd, "\",\"%d\"\r\n", port);//��ʱ���ַ�������������\",\"2000\"\r\n
            //�ϲ�ǰ���ͺ��䣺
            MergeString(SendString, SendStringEnd, 80);//��ʱ���ַ�������������AT+CIPSTART=\"TCP\",\"sys.czbtzn.cn\",\"2000\"\r\n
            NetWorkCommandHandle->SendString = SendString;
            NetWorkCommandHandle->ReturnString = "CONNECT OK\r\n";
            NetWorkCommandHandle->DebugString = "TCP connecting 100%\r\n";
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;  
            //��ʼ�������ݣ�
            if (ERROR_NONE != (returnStatus = NetworkSendTCPSendCommand(NetWorkCommandHandle, data, dataLength)))
            {
                NetWorkCommandHandle->DebugString = "TCP send failed";
                break;             
            }
        }
        while(0);
        //���۷��ͳɹ���ʧ�ܣ���Ҫ�ر����ӣ�
        NetWorkCommandHandle->ReturnString = "OK\r\n";
        NetWorkCommandHandle->SendString = "AT+CIPCLOSE=1\r\n";
        NetWorkCommandHandle->DebugString = "End TCP connection 50%\r\n";
        SIM800A_SendSampleCommand(NetWorkCommandHandle);
        NetWorkCommandHandle->SendString = "AT+CIPSHUT\r\n";
        NetWorkCommandHandle->DebugString = "End TCP connection 100%\r\n";
        SIM800A_SendSampleCommand(NetWorkCommandHandle);
        if (returnStatus == ERROR_NONE)
            return ERROR_NONE;
        else
        {
            retryCnt++;
            USARTSendString(USART_DEBUG, "TCP retrying\n");
        }
    }
    return returnStatus;
}
/*****************************************************************************/
// FUNCTION NAME: NetworkTCPSendCommand
//
// DESCRIPTION:
//  TCP����������������������ν���������ʹ�ü򵥵�����ͺ���
//
// CREATED: 2017-11-02 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
Status NetworkSendTCPSendCommand(SIM800A_QueryTypeDef * NetWorkCommandHandle, u8 * data, int dataLength)
{
    Status returnStatus;
    char sendString[20];
    sprintf(sendString, "AT+CIPSEND=%d\r\n", dataLength);
    NetWorkCommandHandle->SendString = sendString;
    NetWorkCommandHandle->ReturnString = "> ";
    NetWorkCommandHandle->DebugString = "Start sending data\r\n";
    do
    {
        if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle))) 
            break;    //�����һ�ν���ʧ�ܣ���ֱ���˳����ر�����
        USARTSendArray(NetWorkCommandHandle->GPRSUSART, data, dataLength); //��������
        u8 waitCnt = 0;
        OS_Q_DATA ReceiveMsgQData;
        u8 error;
        u8 * receiveString;
        OSTimeDlyHMSM(0, 0, 0, NetWorkCommandHandle->delayMs);
        while(1)
        {
            if (waitCnt < 5)
            {         
                OSQQuery(NetWorkCommandHandle->MsgQ, &ReceiveMsgQData);
                if(ReceiveMsgQData.OSNMsgs > 0)
                {
                    receiveString = OSQPend(NetWorkCommandHandle->MsgQ, 0, &error);		//��Ϣ����
                    if(strcmp((char *)receiveString, "SEND OK\r\n") == 0)		//�����Ϣƥ��
                    {
                        ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);                       //�����Ϣ������
                        NetWorkCommandHandle->DebugString = "TCP success"; //���ݲ�����ʾ������Ϣ
                        USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);  
                        returnStatus = ERROR_NONE;
                        break;
                    }
                    else
                    {
                        ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);  
                    }                
                }
                OSTimeDlyHMSM(0, 0, 0, NetWorkCommandHandle->delayMs);
                waitCnt++;
            }
            else//�ȴ�ʱ�����
            {
                NetWorkCommandHandle->DebugString = "Failed:send data\n"; //���ݲ�����ʾ������Ϣ
                USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);  
                returnStatus = ERROR_OVERTIME;
                break;
            }
        }
    }
    while(0);
    return returnStatus;
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_TCPHandleInit
//
// DESCRIPTION:
//  ��ʼ��TCP�ṹ�壬ע�⣬����ֵҲ�Ǹ�ָ�룬�����ȷ���ռ�ſ���ʹ��
//
// CREATED: 2017-11-22 by zilin Wang
//
// FILE: SIM800A.c
/*****************************************************************************/
void SIM800A_TCPHandleInit(TCP_TypeDef * TCPHandle, char* ipString, int port, char* data, int dataLength, char * returnData, int returnDataLength)
{
    TCPHandle->data = data;
    TCPHandle->dataLength = dataLength;
    TCPHandle->returnData = returnData;
    TCPHandle->returnDataLength = returnDataLength;
    TCPHandle->ipString = ipString;
    TCPHandle->port = port;
}
Status SIM800A_SetUpTCPConnection(SIM800A_QueryTypeDef * NetWorkCommandHandle, TCP_TypeDef * TCPHandle)
{
    Status returnStatus = ERROR_NONE;
    u8 retryCnt = 0;    
    while(retryCnt < NetWorkCommandHandle->retryTimes)
    {
        do
        {
            NetWorkCommandHandle->SendString = "ATE0\r\n";
            NetWorkCommandHandle->ReturnString = "OK\r\n";
            NetWorkCommandHandle->DebugString = "TCP connecting 33%\r\n";
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;          
            NetWorkCommandHandle->SendString = "AT+CLPORT=\"TCP\",\"4000\"\r\n";
            NetWorkCommandHandle->DebugString = "TCP connecting 66%\r\n";
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;
            /******************************************************************************
            ����TCP���������ã�
            ******************************************************************************/
            char SendString[80] = "AT+CIPSTART=\"TCP\",\"";
            //��ǰ����з���url��ip��
            MergeString(SendString, TCPHandle->ipString, 80);//��ʱ���ַ�������������AT+CIPSTART=\"TCP\",\"sys.czbtzn.cn
            //�ں����з���˿ں�
            char SendStringEnd[20];
            sprintf(SendStringEnd, "\",\"%d\"\r\n", TCPHandle->port);//��ʱ���ַ�������������\",\"2000\"\r\n
            //�ϲ�ǰ���ͺ��䣺
            MergeString(SendString, SendStringEnd, 80);//��ʱ���ַ�������������AT+CIPSTART=\"TCP\",\"sys.czbtzn.cn\",\"2000\"\r\n
            NetWorkCommandHandle->SendString = SendString;
            NetWorkCommandHandle->ReturnString = "CONNECT OK\r\n";
            NetWorkCommandHandle->DebugString = "TCP connecting 100%\r\n";
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;  
        }
        while(0);      
        if (returnStatus == ERROR_NONE)
            return ERROR_NONE;
        else
        {
            //����޷����ӣ���ر�TCP
            NetWorkCommandHandle->SendString = "AT+CIPSHUT\r\n";
            NetWorkCommandHandle->DebugString = "TCP retrying\n";
            USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString); 
            SIM800A_SendSampleCommand(NetWorkCommandHandle);
            retryCnt++;
        }
    }
    return returnStatus;
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_TCPCommunication
//
// DESCRIPTION:
//  TCP����һ�ν�����ע�⣬���ʧ�ܲ�������
//
// CREATED: 2017-11-22 by zilin Wang
//
// FILE: SIM800A.c
/*****************************************************************************/
Status SIM800A_TCPCommunication(SIM800A_QueryTypeDef * NetWorkCommandHandle, TCP_TypeDef * TCPHandle)
{
    //��֤������ȷ�ԣ�
    if (TCPHandle->data == NULL)
        return ERROR_NULLPOINTER;
    Status returnStatus;
    char sendString[30];
    sprintf(sendString, "AT+CIPSEND=%d\r\n", TCPHandle->dataLength);
    NetWorkCommandHandle->SendString = sendString;
    NetWorkCommandHandle->ReturnString = "> ";
    NetWorkCommandHandle->DebugString = "Start sending data\r\n";
    do
    {
        if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle))) 
            break;    //�����һ�ν���ʧ�ܣ���ֱ���˳����ر�����
        USARTSendArray(NetWorkCommandHandle->GPRSUSART, (u8*)TCPHandle->data, TCPHandle->dataLength); //��������
        u8 waitCnt = 0;
        OS_Q_DATA ReceiveMsgQData;
        u8 error;
        u8 * receiveString;
        OSTimeDlyHMSM(0, 0, 0, NetWorkCommandHandle->delayMs);
        while(1)
        {
            if (waitCnt < 5)
            {         
                OSQQuery(NetWorkCommandHandle->MsgQ, &ReceiveMsgQData);
                if(ReceiveMsgQData.OSNMsgs > 0)
                {
                    receiveString = OSQPend(NetWorkCommandHandle->MsgQ, 0, &error);		//��Ϣ����
                    if(strcmp((char *)receiveString, "SEND OK\r\n") == 0)		//�����Ϣƥ��
                    {
                        ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);                       //�����Ϣ������
                        NetWorkCommandHandle->DebugString = "TCP success\n"; //���ݲ�����ʾ������Ϣ
                        USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);  
                        returnStatus = ERROR_NONE;
                        break;
                    }
                    else
                    {
                        ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);  
                    }                
                }
                OSTimeDlyHMSM(0, 0, 0, NetWorkCommandHandle->delayMs);
                waitCnt++;
            }
            else//�ȴ�ʱ�����
            {
                NetWorkCommandHandle->DebugString = "Failed:send data"; //���ݲ�����ʾ������Ϣ
                USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);  
                returnStatus = ERROR_OVERTIME;
                break;
            }
        } 
        if (TCPHandle->returnData != NULL)//�ȴ����ص���Ϣ������еĻ���
        {
            waitCnt = 0; 
            while(1)
            {
                if (waitCnt < 15)
                {         
                    OSQQuery(NetWorkCommandHandle->MsgQ, &ReceiveMsgQData);
                    if(ReceiveMsgQData.OSNMsgs > 0)
                    {
                        receiveString = OSQPend(NetWorkCommandHandle->MsgQ, 0, &error);		//��Ϣ����
                        while(*receiveString != '\n')
                        {
                            *(TCPHandle->returnData) = *receiveString;
                            TCPHandle->returnData++;
                            receiveString++;             
                        }
                        *(++TCPHandle->returnData) = '\0';
                        ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);
                        returnStatus = ERROR_NONE;
                        break;
                    }
                    OSTimeDlyHMSM(0, 0, 0, NetWorkCommandHandle->delayMs);
                    waitCnt++;
                }
                else//�ȴ�ʱ�����
                {
                    NetWorkCommandHandle->DebugString = "Failed:TCP no response"; //���ݲ�����ʾ������Ϣ
                    USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);  
                    returnStatus = ERROR_NORESPONSE;
                    break;
                }
            }
        }
    }
    while(0);
    return returnStatus;
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_DisConnectTCPConnection
//
// DESCRIPTION:
//  �Ͽ�TCP����
//
// CREATED: 2017-11-22 by zilin Wang
//
// FILE: SIM800A.c
/*****************************************************************************/
void SIM800A_DisConnectTCPConnection(SIM800A_QueryTypeDef * NetWorkCommandHandle)
{
    NetWorkCommandHandle->retryTimes = 1;
    NetWorkCommandHandle->ReturnString = "OK\r\n";
    NetWorkCommandHandle->SendString = "AT+CIPCLOSE=1\r\n";
    NetWorkCommandHandle->DebugString = "End TCP connection 50%\r\n";
    SIM800A_SendSampleCommand(NetWorkCommandHandle);
    NetWorkCommandHandle->SendString = "AT+CIPSHUT\r\n";
    NetWorkCommandHandle->DebugString = "End TCP connection 100%\r\n";
    SIM800A_SendSampleCommand(NetWorkCommandHandle);
    NetWorkCommandHandle->retryTimes = 3;//�ر����Ӳ��������������
}
