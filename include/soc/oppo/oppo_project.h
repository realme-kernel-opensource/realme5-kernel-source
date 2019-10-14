/* 
 *
 * yixue.ge add for oppo project
 *
 *
 */
#ifndef _OPPO_PROJECT_H_
#define _OPPO_PROJECT_H_

enum{
        HW_VERSION__UNKNOWN,
        HW_VERSION__10,   /*1452mV*/
        HW_VERSION__11,   /*1636 mV*/
        HW_VERSION__12,   /*1224 mV*/
        HW_VERSION__13,   /*900 mV*/
        HW_VERSION__14,   /*720 mV*/
        HW_VERSION__15,
        HW_VERSION__16,
};


enum{
        RF_VERSION__UNKNOWN,
        RF_VERSION__11,
        RF_VERSION__12,
        RF_VERSION__13,
        RF_VERSION__14,
        RF_VERSION__15,
        RF_VERSION__16,
        RF_VERSION__17,
        RF_VERSION__18,
        RF_VERSION__19,
        RF_VERSION__1A,
};


#define GET_PCB_VERSION() (get_PCB_Version())
#define GET_PCB_VERSION_STRING() (get_PCB_Version_String())

#define GET_MODEM_VERSION() (get_Modem_Version())
#define GET_OPERATOR_VERSION() (get_Operator_Version())



enum OPPO_PROJECT {
        OPPO_UNKOWN = 0,
        OPPO_18081 = 18081,
        OPPO_18085 = 18085,
        OPPO_18181 = 18181,
	OPPO_19631 = 19631,
        OPPO_11870 = 11870,
        OPPO_18115 = 18115,
        OPPO_18501 = 18501,
        OPPO_18503 = 18503,
        OPPO_19021 = 19021,
        OPPO_19026 = 19026,
        OPPO_19321 = 19321,
	OPPO_19633 = 19633,
	OPPO_19632 = 19632,
	OPPO_19637 = 19637,
	OPPO_19638 = 19638,
};

enum OPPO_OPERATOR {
        OPERATOR_UNKOWN                     = 0,
        OPERATOR_OPEN_MARKET          = 1,
        OPERATOR_CHINA_MOBILE             = 2,
        OPERATOR_CHINA_UNICOM              = 3,
        OPERATOR_CHINA_TELECOM                = 4,
        OPERATOR_FOREIGN                             = 5,
/*#ifdef VENDOR_EDIT*/
/*TongJing.Shi@EXP.DataComm.Phone, 2014.04.18, Add for exp just 3G no 4G*/
        OPERATOR_FOREIGN_WCDMA         = 6,   /*qifeng.liu 2014.08.07 FOR MAC*/
        OPERATOR_FOREIGN_RESERVED   = 7,    /*shaoming 2014/10/04 add for 14085's dual sim version*/
        OPERATOR_ALL_CHINA_CARRIER    = 8,   /*instead of TELECOM CARRIER because of history Tong.han@Bsp.Group.Tp add for all china carrier phone, 2015/03/23*/
        OPERATOR_ALL_CHINA_CARRIER_MOBILE = 9,    /*rendong.shi@Bsp.Group.Tp add for all china carrier MOBILE phone, 2016/01/07*/
        OPERATOR_ALL_CHINA_CARRIER_UNICOM = 10,    /*rendong.shi@Bsp.Group.Tp add for all china carrier UNICOM  phone, 2016/01/07*/
        OPERATOR_FOREIGN_EUROPE = 11,    //wanghao@Bsp.Group.Tp add for foreign europe  phone, 2018/07/14
        OPERATOR_FOREIGN_ASIA = 12,    //tanyang@Bsp.Group.sec add for foreign    phone, 2019/04/23
        OPERATOR_FOREIGN_ASIA_SIMPLE = 13,    //tanyang@Bsp.Group.sec add for foreign    phone, 2019/04/23
        OPERATOR_FOREIGN_FULL_BAND = 14,    //tanyang@Bsp.Group.sec add for foreign    phone, 2019/04/23
        OPERATOR_FOREIGN_CARRIER = 15,    //tanyang@Bsp.Group.sec add for foreign     phone, 2019/04/23
};

typedef enum OPPO_PROJECT OPPO_PROJECT;
#define OCPCOUNTMAX 4
typedef struct
{
	unsigned int                  nproject;
	unsigned char                 nmodem;
	unsigned char                 noperator;
	unsigned char                 npcbversion;
	unsigned char                 noppobootmode;
	unsigned char                 npmicocp[OCPCOUNTMAX];
} ProjectInfoCDTType;

#ifdef CONFIG_OPPO_COMMON_SOFT
void init_project_version(void);
unsigned int get_project(void);
unsigned int is_project(OPPO_PROJECT project);
unsigned char get_PCB_Version(void);
unsigned char get_Modem_Version(void);
unsigned char get_Operator_Version(void);
#else
unsigned int init_project_version(void) { return 0;}
unsigned int get_project(void) { return 0;}
unsigned int is_project(OPPO_PROJECT project) { return 0;}
unsigned char get_PCB_Version(void) { return 0;}
unsigned char get_Modem_Version(void) { return 0;}
unsigned char get_Operator_Version(void) { return 0;}
#endif
#endif  /*_OPPO_PROJECT_H_*/

