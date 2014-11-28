//���õ�ͷ�ļ�
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <malloc.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include "freetdstodb.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef LINUX
#include <sys/epoll.h>
#else
#include <poll.h>
#endif

//#define DBSERVER "192.168.1.128:2816"   //���ݿ���������ӵ�ַ
//#define SQL_DBNAME "OAInstantTalk"  //���ݿ���

/***************************************************************************************/

//���ݿ��¼
//#define SQL_USER "sa"   //�û���
//#define SQL_PASSWD "123456" //����

//CGIͨ��
//#define MAX_CGI_LINK 350    //CGI���������
//#define UNIX_PATH "/cygdrive/c/Apache2.2/cgi-bin/cgi_link"    //���׽������ӷ�·��(windows�û�����������/dev/fs/�����ж�дȨ�޵�Ŀ¼�ڣ��׽ӿڵ����Ʊ�������ļ������������Ҳ����к�׺���ú��CGIͷ�ļ��е�ͬ���������ͬ)

//�м��ͨ��
//#define MI_PORT 4220    //�м���˿�
//#define MI_ADDR "192.168.1.128" //�м��IP
#define DATA_BUF_LEN 1024*4 //�м���շ�������

//XMLͷ
#define MO_XML_HEAD "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
//#define CONPANY_ID 1    //��˾ID

/***************************************ҵ�����************************************/
#define BUS_SERVER_PUSH 0   //������������������
#define BUS_LOGIN 1 //�û���¼
#define BUS_CANCELLATION 2  //�û�ע��
#define BUS_INDIVIDUAL_SESSION 31   //���˻Ự
#define BUS_GROUP_SESSION 32    //Ⱥ�Ự
#define BUS_MULTIPLAYER_SESSION 33  //���˻Ự
#define BUS_OL_MOBILE 41    //��ȡ�ƶ������߳�Ա�б�
#define BUS_OL_PC 42    //��ȡPC�����߳�Ա�б�
#define BUS_ORG_STU 5   //��ȡ��֯�ṹ
#define BUS_GROUPER 6   //��ȡȺ��Ա
#define BUS_GROUP_SHIELD 7  //Ⱥ��Ϣ����
#define BUS_GROUP 8 //��ȡ�û�����Ⱥ����Ϣ
#define BUS_AVATAR 9    //��ȡ�û�ͷ��
#define BUS_LOGINER_MSG 10  //��ȡ�û�������Ϣ
#define BUS_MULTIPLAYER 11  //��ȡ�������Ա
#define BUS_MULTI 12    //��ȡ�û��������������Ϣ
#define BUS_NEW_GROUP 13    //�½�Ⱥ
#define BUS_NEW_MULTIPLAYER_SESSION 14  //�½����˻Ự
#define BUS_ADD_MULTIPLAYER_SESSION 15  //���Ӷ��˻Ự��Ա
#define BUS_EXIT_GROUP 16   //�˳�Ⱥ
#define BUS_EXIT_MULTI 17   //�˳����˻Ự
#define BUS_UPDATE_LOGINER_MSG 18   //�޸ĸ�������
#define BUS_CON_PIC 19  //��ȡ�Ự�����е�ͼƬ
#define BUS_CHE_PHOTO 20	//��֤�û�ͷ��
/***********************************************************************************/

enum GroupMutil
{
    CTRLALL=0,
    CTRLPERSON,
    CTRLGROUP,
    CTRLMUTIL,
};

enum TRANSTYPE
{
    SYN = 0x0,
    ACK = 0x1,
};

enum TRANSRESULT
{
    RET_SUCCEED = 0x0,
    RET_FAILD = 0x1,
    RET_REFUSE = 0x2,
};

// Ӧ�ò�����״̬��
enum BusinessState  //unsigned char
{
    REQUEST,    // ����
    RESPONSE,   // ��Ӧ

    RECORDLINK, // ��¼�����ϵ��

    IDORPASSWORD,   // ��¼ʱ��ID�����������
    BS_NOBOUND, // ���ص�¼ʱ��δ��ԭ���˺�

    FAILD,  // ʧ��
    NOSUPPORT,  // ��֧�ִ�����

    REFUSE, // �ܾ���Ŀǰ���ھܾ���Ⱥ����
};

// �������ͣ�
enum EncryptType    //unsigned char
{
    NONE,   // ������
    RSA128,
    RSA256,
    DES24,  // 3DES����
};

// �м��ҵ���
enum MiddleCommand  //short
{
    MC_EXTAPP_CONN, // ��չӦ���������м��
    MC_EXTAPP_SCHEMA,   // ��չӦ�ý��Լ��������б������м��
    MC_EXTAPP_GETONLINELIST,    // ��չӦ�ô��м����ȡָ������������б�

    MC_BTPC_LOGIN = 0x100,  // ����PC�˵�¼
    MC_BTPC_LOGOUT,         // ����PC�˵ǳ�
    MC_BTPC_CTM_BASEINFO,   // �û��޸�������������
    MC_BTPC_CTM_PHOTO,      // �û��޸�������Ƭ
    MC_BTPC_CTM_MOOD,       // �û���������
    MC_BTPC_PTOP_MSG,       // ���˵�Ե�ͨ����Ϣ
    MC_BTPC_GROUP_CREATE,   // Ⱥ�鴴��
    MC_BTPC_GROUP_DELETE,   // Ⱥ���ɢ
    MC_BTPC_GROUP_ADDUSER,  // Ⱥ�����
    MC_BTPC_GROUP_DELUSER,  // Ⱥ��ɾ��
    MC_BTPC_GROUP_NOTICE,   // Ⱥ�����޸�
    MC_BTPC_GROUP_NOTIFY,   // �û��޸�Ⱥ��֪ͨ����
    MC_BTPC_GROUP_MSG,      // Ⱥ��Ϣ
    MC_BTPC_MULTI_CREATE,   // �����鴴��
    MC_BTPC_MULTI_ADDUSER,  // ���������
    MC_BTPC_MULTI_DELUSER,  // ������ɾ��
    MC_BTPC_MULTI_SUBJECT,  // �޸�����������
    MC_BTPC_MULTI_MSG,      // ��������Ϣ

    MC_BTANDRIOD_LOGIN = 0x200, // ���İ�׿�˵�¼
    MC_BTANDRIOD_LOGOUT,        // ���İ�׿�˵ǳ�
    MC_BTANDRIOD_CTM_BASEINFO,  // �û��޸�������������
    MC_BTANDRIOD_CTM_PHOTO,     // �û��޸�������Ƭ
    MC_BTANDRIOD_CTM_MOOD,      // �û���������
    MC_BTANDRIOD_PTOP_MSG,      // ���˵�Ե�ͨ����Ϣ
    MC_BTANDRIOD_GROUP_CREATE,  // Ⱥ�鴴��
    MC_BTANDRIOD_GROUP_DELETE,  // Ⱥ���ɢ
    MC_BTANDRIOD_GROUP_ADDUSER, // Ⱥ�����
    MC_BTANDRIOD_GROUP_DELUSER, // Ⱥ��ɾ��
    MC_BTANDRIOD_GROUP_NOTICE,  // Ⱥ�����޸�
    MC_BTANDRIOD_GROUP_NOTIFY,  // �û��޸�Ⱥ��֪ͨ����
    MC_BTANDRIOD_GROUP_MSG,     // Ⱥ��Ϣ
    MC_BTANDRIOD_MULTI_CREATE,  // �����鴴��
    MC_BTANDRIOD_MULTI_ADDUSER, // ���������
    MC_BTANDRIOD_MULTI_DELUSER, // ������ɾ��
    MC_BTANDRIOD_MULTI_SUBJECT, // �޸�����������
    MC_BTANDRIOD_MULTI_MSG,     // ��������Ϣ

    MC_BTANDRIOD_PTOP_IMAGE = 0x300,        // ���˵�Ե�ͼƬ��Ϣ
    MC_BTANDRIOD_GROUP_IMAGE,
    MC_BTANDRIOD_MULTI_IMAGE,
};

enum ExtAppType
{
    EAT_NOTREG, // δ֪�ķ�������
    EAT_BTPCINSTANT,    // ����PC�漴ʱͨѶ����
    EAT_BTANDRIOD,  // ���İ�׿�����
};

enum GroupNotifyType
{
    GNT_RECVANDPOPUP,   //���ղ�����
    GNT_RECVNOTPOPUP,   //���ղ�����
    GNT_REFUSE, //����
};

/***********************************�м������*******************************************/


typedef struct  // ҵ�����ͷ
{
    uint16_t Command; // ҵ���
    uint8_t Result;   // ҵ��ִ�н��
    uint8_t Enctype;  // ������ļ�������
    uint32_t Datalen;   // ������ʵ�ʳ���
    uint32_t Srclen;    // ���������ĵ�Դ���ݵĳ��ȣ���û�м�����Ӧ��Datalen��ֵ
} BsnsPacket,*BsPt;

typedef struct  //�������ͷ
{
    union
    {
        uint16_t Type:4;  // �����ͣ������/��Ӧ��
        uint16_t Ret:4;   //
        uint16_t Window:8;// ����/���մ���
        uint16_t Flag;
    } State;
    uint16_t Count;   // �ְ�����
    uint16_t Index;   // �ְ���������0��ʼ
    uint16_t Datalen;// �����򳤶�
    uint16_t DataVerfy;   // ������CRC
    uint16_t HeadVerfy;   // ��ͷCRC
} TransPacket,*TsPt;

//����ҵ���
typedef struct
{
    BsnsPacket bp;
    char id[32];
    char context[0];
} MS32CHARINFO,*MS32_CHARINFO;

//1)�û���¼��idΪ��¼�û�ID
//2���û�ע����idΪע���û�ID
//3�������û��������ϣ�idΪ�û�ID

typedef struct
{
    BsnsPacket bp;
    char srcid[32];
    char desid[32];
    char context[0];
} MS64CHARINFO,*MS64_CHARINFO;

typedef struct
{
    char srcid[32];
    char desid[32];
    char context[0];
} RMS64CHARINFO,*RMS64_CHARINFO;


//1�����˻Ự��srcidΪ������ID��desidΪĿ����ID��contextΪ�Ự����,ע�⣬�Ự����Ϊ����URL�����UTF-8�ַ���

typedef struct
{
    BsnsPacket bp;
    uint32_t ntype;
    char context[0];
} MSEXTAPPREG_REQ,*MSEXTAPP_REG_REQ;

//1������˵�¼��ntypeΪEAT_BTANDRIOD
//2������˷����Լ��������û��б�,contextΪ�û��б�XML������û���û�ʱ����ֻ��XMLͷ
//3����������м������PC����������б�XML
//4) ��Ⱥ/�����鴴����ɾ��,ntypeΪ��Ⱥ/������ID��PC���ƶ���ͬ��
//5����������ˣ�ntypeΪ������ID��contextΪ�����ӵ��û�ID����ʽΪ:�û�ID|����:�û�AID|�û�BID|�û�CID|

//6���м�������˷���PC�˵������б�XML

typedef struct
{
    BsnsPacket bp;
    char uid[32];
    uint32_t gid;
    uint32_t opertype;
    char context[0];
} MSGROUPCTRL,*MS_GROUP_CTRL;

typedef struct
{
    char uid[32];
    uint32_t gid;
    uint32_t opertype;
    char context[0];
} RMSGROUPCTRL,*RMS_GROUP_CTRL;


//1)����Ⱥ��Ϣ��uid�û�ID��gidΪȺ�ţ�opertypeΪ����/������

typedef struct
{
    BsnsPacket bp;
    char uid[32];
    uint32_t gid;
    char context[0];
} MSGROUP,*MS_GROUP;

typedef struct
{
    char uid[32];
    uint32_t gid;
    char context[0];
} RMSGROUP,*RMS_GROUP;


//1���˳�Ⱥ/�����飬uidΪ�û�ID��gidΪȺ/������ID
//2��������/Ⱥ��Ự��uidΪ�Ự�û�ID��gidΪ������/Ⱥ��ID��contextΪ�Ự���ݣ�ע�⣬�Ự����Ϊ����URL�����UTF-8�ַ���

//3���м�����ѷ���ˣ�Ⱥ/��������Ϣ��gidΪȺ/������ID��uidΪ������ID

typedef struct
{
    uint32_t ntype;
    uint32_t len;
    char context[0];
} REXML,*RE_XML;

/***************************************************************************************/


/*******************************************CGI����*************************************/
typedef struct  //CGI��������Ϣ
{
    uint32_t packet_len;    //����
    uint32_t type;  //ҵ���
    char sender[32];    //�û�id
    char recver[32];    //Ŀ��id�����ڶԻ�
    uint32_t len;   //context�ĳ���
    char context[0];    //ҵ����Ϣ
} CGI_MSG,*CM;

typedef struct  //��Ӧ��CGI����Ϣ
{
    uint32_t len;   //���ݳ���
    char context[0];    //�����ı�
} CGI_RM_MSG,*CRM;

typedef struct Depart
{
    int did;
    char dpm[128];
    struct Depart *next;
} subdpm,*subdpmt;

struct _DPMNODE	// ���Žڵ���Ϣ
{
	int DpmId;
	char DpmName[128];
};

/***************************************************************************************/

typedef struct imf_list	//֪ͨ���нڵ�
{
    struct imf_list *next;
    uint len;
    char* context[0];
} IMF_LIST,*IL;

typedef struct user_link    //�û����߽ṹ�壬�����������
{
    char id[32];    //�û�id
    int fd; //CGI���׽����ļ�������
    //pthread_t time; //��ʱ�߳�(8byte)
    int flag;
    IL il;
    struct user_link *next;
} User_Linking,*UL;

/*************************************ȫ�ֱ���******************************************/

pthread_mutex_t mutex,mutex_ol,mutex_cgi; //�߳�������ֹ����߳�ͬʱ������Ϣ���м����CGI
//pthread_mutex_t mutex_sql;  //��ֹ����߳�ͬʱ��д���ݿ�
int mi_fd;  //�м���׽���
int cgi_fd; //CGI���׽��ַ�����ļ�������
UL user;    //�û���������
sem_t mi_send_recv_ctrl;    //�м���������ӿ���
pthread_t rad_thread;	//�м�������߳�

char* org_stu;  //��֯�ṹ
char* tmp_stu;  //��ʱ��֯�ṹ������

int g_nOrgStuLen;	// ��֯�ṹ����������

char *PC_OL;    //PC�������б�
char *MO_OL;    //�ƶ��������б�

//���ݿ�������
//DBPROCESS *dbprocess;

//���ݿ�����
char* DBSERVER;  //���ݿ���������ӵ�ַ
char* SQL_DBNAME;  //���ݿ���
uint MAX_DATA;	//���Ի�ȡ���ݿ�����������
char* SQL_USER;   //�û���
char* SQL_PASSWD; //����
uint SQL_LINK_COUNT;	//���ݿ����ӳ�

//CGIͨ��
int MAX_CGI_LINK;   //CGI���������
char* UNIX_PATH;    //���׽������ӷ�·��(�û����������ж�дȨ�޵�Ŀ¼�ڣ��׽ӿڵ����Ʊ�������ļ������������Ҳ����к�׺���ñ�����CGIͷ�ļ��е�ͬ���������ͬ)

//�м��ͨ��
int MI_PORT;    //�м���˿�
char* MI_ADDR; //�м��IP

int CONPANY_ID;    //��˾ID

/***************************************************************************************/

/***********************************************net.c***********************************/
int Init_Mi_TCP(char* ip,int port); //��ʼ�������м��
int Exit_Mi_TCP(); //�����м������
void* MI_Read(void* args);  //�м�������߳�
int MI_Write(const char *bsnsdata, int len,int flag);    //���м��������ҵ������ͺ���
void HandleBusiness(BsPt pBsns, const char *data, uint len);  // ���м��������ҵ�����ݴ�������

/*****************************************************************************************/

/**************************************protocol.c*****************************************/

TsPt calcULate_verfy(TsPt tp,unsigned char *data, unsigned short len);  //��ʼ�������
BsnsPacket BsnsPacket_init(unsigned short cmd, unsigned char ret, unsigned char enctype,int len);   //��ʼ��ҵ���

/*****************************************************************************************/

/*****************************************cgi_ctrl.c*****************************************/

void setnonblocking(int sock);  //�趨�������׽���
void* Flush_CGI(void* arg); //�����߳�
void* CGI_Link(void* nfd);  //CGI�Ự����
int Zero_RE(int fd,char* context,int len,int type);  //0ҵ��Ļط�������ֵΪ-1ʱʧ�ܣ�����ֵ��Ϊ�ɹ�
void* Check_OutLine(void* nfd); //�����ж�
CRM Business_deal(CM cm,int fd);    //CGIҵ����
void cgi_all_send(char* msg,char* me);   //�ͻ��˹㲥֪ͨ

/*****************************************************************************************/

/*****************************************cgi_business.c**********************************/

char* Check_Logined(char* loginer); //����¼
char* Login(char* loginer,char* password);  //��¼
char* Cancellation(char* loginer);   //ע��
char* GetOrg_Stu(); //��ȡ��֯�ṹ
char* GetPc_Ol();   //��ȡPC���û������б�
char* GetMo_Ol();   //��ȡ�ƶ����û������б�
char* ServerPush(char* loginer,char* msg,int fd,UL ul);   //����������Ԥ������
char* GetGrouper(char* gid);    //��ȡȺ��Ա�б�
char* SetShield(char* loginer,char* context);   //����Ⱥ��Ϣ����
char* GetGroup(char* loginer);  //��ȡȺ�б�
char* GetAvatar(char* loginer); //��ȡͷ��
char* GetOrger_Msg(char* orger);    //��ȡ��������
char* GetMultiplayer(char* mid);    //��ȡ�������Ա�б�
char* GetMulti(char* loginer);  //��ȡ�������б�
char* NewGroup(char* loginer,char* context);    //�½�Ⱥ
char* NewMulti(char* loginer,char* context);    //�½�������
char* AddMulti(char* context);  //�����������Ա
char* ExitGroup(char* loginer,char* gid);   //�˳�Ⱥ
char* ExitMulti(char* loginer,char* mid);   //�˳����˻Ự
char* UpdateLoginerMsg(char* loginer,char* context);    //�޸ı��˸�������
char* GetPicture(char* pid);    //��ȡ�Ի������е�ͼƬ
char* Talk(int type,char* src,char* des,char* context,uint32_t len);    //���ͻỰ
char* GetImf(UL ul,char* loginer); //��ȡ֪ͨ����
void strrpl(char* pDstOut, char* pSrcIn, const char* pSrcRpl, const char* pDstRpl); //�ַ����滻
int Count(char *const a,char *const b); //���ַ���ͳ��
char* Check_Photo(char* uid,char* md5);	//��֤ͷ��

//char* GetImportList(char* id,char* context);	//��ȡ֪ͨ�����б�
//char* GetImportDetail(char* id,char* context);	//��ȡ֪ͨ������ϸ

/***************************************************************************************/

/****************************************mi_business.c************************************/

void OnGetOnlineList(unsigned char Result, char* srcdata, int srclen);  //��ȡPC�������б�
int MSG_RECV(unsigned char Result,char* srcdata,int srclen,int type);   //��ȡPC����Ϣ
void MSG_INFO(unsigned char Result,int type);   //��ȡPC��֪ͨ
char Char2Int(char ch); //�ַ�ת����
char Str2Bin(char *str);    //�ַ�ת������
unsigned char* UrlDecode(char* str);    //URL����
int Link_Mi();  //�����м��ȷ�ϲ������Լ������߳�Ա�б�
int SEND_GET_PC_ONLINE_LIST();  //��ȡPC�����߳�Ա�б�
void OnMiddleLogin(unsigned char result);   //�����¼�м��
void OnMiddleSchema(unsigned char result);  //���м����������,����PC�����б�
int Send_MO_OL();   //�����ƶ��������б�

/***************************************************************************************/

/**************************************freetds.c(LINUX)******************************************/

//int ConnectToDB(char* server,char* userid,char* passwd,char* dbname);   //establishes the connection with the DB
//void CloseConnection(); //�ر�����
//int CTRLDB(char* SQL_CTRL); //�������

/********************************************************************************************************************/

/******************************************************dbctrl.c******************************************************/

int check_up(char *uid,char *pass); //ȷ���û��Ѿ�����
int set_group(char *uid,int gid,int type);  //����Ⱥ����
int insert_group(char* uid,char* name,char* theme,char* id,int* gid,int type);  //����Ⱥ/������
int insert_mutil_orger(int gid,char* ids);  //�����������Ա
int update_org_info(char* uid,char* Phone,char* Mobile,char* Mail,char* Mood);  //�޸ĳ�Ա��Ϣ
int exit_group_mutil(char* uid,int gid,int type);   //�˳�Ⱥ/������
int delete_msg(char* ids,int type); //ɾ��������Ϣ
int insert_talklist(char* src,char* des,char* context,uint32_t len,int type);   //�����Ự��Ϣ
int check_user_group_or_mutil(int gid,char* uid,int type);  //ȷ���û���ĳ��Ⱥ/��������
//int get_org_stu(int dpmid); //��ȡ��֯�ṹ
//int get_subdepartmemt_user(char* departmemt,int did);   //��ȡ����������֯�ṹ
char* get_group_or_mutil(char* id,int type);    //��ȡȺ/������
char* get_member(int gid,int type); //��ȡȺ/�������Ա
char* get_info(char* uid);  //��ȡ������Ϣ
char* get_photo(char* uid); //��ȡͷ�������
char* search_info(char* loginer,int type);  //��ȡ������Ϣ
char* get_picture(int pid,char* ext);   //��ȡ�Ի�ͼƬ
int get_group_mutil_user(char* uid,int gid,int type);   //��ȡȺ/���˻Ự��Ա������֪ͨ
int check_user_photo(char* uid,char* md5);	//��֤���ݿ���photo��MD5�ֶ�
//��ȡ��֯�ṹ
int GetOnlineCtms(int dpmid);
int GetSubDpmsAndWorkersForCvst(int dmpid);

/********************************************************************************************************************/

/********************************************userlink.c*********************/

UL get_point(UL head,char* loginer);    //��ȡ�����û��ڵ�
int get_len(UL head);   //��ȡ�����û�����
int insert_point(UL head,char* loginer,int fd); //���������û��ڵ㣨��¼��
int delete_point_log(UL head,UL uid);    //ɾ�������û��ڵ�
int update_point_fd(UL head,char* loginer,int fd);  //ˢ�������û������ļ�������
//int update_point_pth(UL head,int fd,pthread_t time);    //�޸��û����������ڵ���߳�������
char* NULL_OL();    //PC��û���û�����
UL get_point_fd(UL head,int fd);//��ȡ�û����������ڵ�
void DeleteList(IL head);   //ɾ��֪ͨ����
void insert_imf(IL head,char* context,uint len);    //����֪ͨ
int delete_imf(IL head,IL uimf);    //ɾ��֪ͨ
IL get_imf(IL head);    //��ȡ֪ͨ
void flush_list(UL head);   //��ȡ��ǰ�����û��б�
void DeleteULList(UL head); //����û������б�

/***************************************************************************************/