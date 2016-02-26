/*
 * G6 - TCP Transfer && LB Dispenser
 * Author      : calvin
 * Email       : calvinwillliams@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#ifndef _H_G6_
#define _H_G6_

#if ( defined __linux ) || ( defined __unix )
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#define _VSNPRINTF			vsnprintf
#define _SNPRINTF			snprintf
#define _CLOSESOCKET			close
#define _CLOSESOCKET2(_fd1_,_fd2_)	close(_fd1_),close(_fd2_);
#define _ERRNO				errno
#define _EWOULDBLOCK			EWOULDBLOCK
#define _ECONNABORTED			ECONNABORTED
#define _EINPROGRESS			EINPROGRESS
#define _ECONNRESET			ECONNRESET
#define _ENOTCONN			ENOTCONN
#define _EISCONN			EISCONN
#define _SOCKLEN_T			socklen_t
#define _GETTIMEOFDAY(_tv_)		gettimeofday(&(_tv_),NULL)
#define _LOCALTIME(_tt_,_stime_) \
	localtime_r(&(_tt_),&(_stime_));
#elif ( defined _WIN32 )
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <io.h>
#include <windows.h>
#define _VSNPRINTF			_vsnprintf
#define _SNPRINTF			_snprintf
#define _CLOSESOCKET			closesocket
#define _CLOSESOCKET2(_fd1_,_fd2_)	closesocket(_fd1_),closesocket(_fd2_);
#define _ERRNO				GetLastError()
#define _EWOULDBLOCK			WSAEWOULDBLOCK
#define _ECONNABORTED			WSAECONNABORTED
#define _EINPROGRESS			WSAEINPROGRESS
#define _ECONNRESET			WSAECONNRESET
#define _ENOTCONN			WSAENOTCONN
#define _EISCONN			WSAEISCONN
#define _SOCKLEN_T			int
#define _GETTIMEOFDAY(_tv_) \
	{ \
		SYSTEMTIME stNow ; \
		GetLocalTime( & stNow ); \
		(_tv_).tv_usec = stNow.wMilliseconds * 1000 ; \
		time( & ((_tv_).tv_sec) ); \
	}
#define _SYSTEMTIME2TIMEVAL_USEC(_syst_,_tv_) \
		(_tv_).tv_usec = (_syst_).wMilliseconds * 1000 ;
#define _SYSTEMTIME2TM(_syst_,_stime_) \
		(_stime_).tm_year = (_syst_).wYear - 1900 ; \
		(_stime_).tm_mon = (_syst_).wMonth - 1 ; \
		(_stime_).tm_mday = (_syst_).wDay ; \
		(_stime_).tm_hour = (_syst_).wHour ; \
		(_stime_).tm_min = (_syst_).wMinute ; \
		(_stime_).tm_sec = (_syst_).wSecond ;
#define _LOCALTIME(_tt_,_stime_) \
	{ \
		SYSTEMTIME	stNow ; \
		GetLocalTime( & stNow ); \
		_SYSTEMTIME2TM( stNow , (_stime_) ); \
	}
#endif

#include "LOGC.h"

#ifndef ULONG_MAX
#define ULONG_MAX 0xffffffffUL
#endif

#define FOUND				9	/* �ҵ� */
#define NOT_FOUND			4	/* �Ҳ��� */

#define MATCH				1	/* ƥ�� */
#define NOT_MATCH			-1	/* ��ƥ�� */

#define LOAD_BALANCE_ALGORITHM_G	"G"	/* ����˿� */
#define LOAD_BALANCE_ALGORITHM_MS	"MS"	/* ����ģʽ */
#define LOAD_BALANCE_ALGORITHM_RR	"RR"	/* ��ѯģʽ */
#define LOAD_BALANCE_ALGORITHM_LC	"LC"	/* ��������ģʽ */
#define LOAD_BALANCE_ALGORITHM_RT	"RT"	/* ��С��Ӧʱ��ģʽ */
#define LOAD_BALANCE_ALGORITHM_RD	"RD"	/* ���ģʽ */
#define LOAD_BALANCE_ALGORITHM_HS	"HS"	/* HASHģʽ */

#define DEFAULT_RULES_INITCOUNT			2
#define DEFAULT_RULES_INCREASE			5

#define DEFAULT_CLIENTS_INITCOUNT_IN_ONE_RULE	2
#define DEFAULT_CLIENTS_INCREASE_IN_ONE_RULE	5
#define DEFAULT_FORWARDS_INITCOUNT_IN_ONE_RULE	2
#define DEFAULT_FORWARDS_INCREASE_IN_ONE_RULE	5
#define DEFAULT_SERVERS_INITCOUNT_IN_ONE_RULE	2
#define DEFAULT_SERVERS_INCREASE_IN_ONE_RULE	5

#define DEFAULT_FORWARD_SESSIONS_MAXCOUNT	10000	/* ȱʡ����������� */
#define DEFAULT_FORWARD_TRANSFER_BUFSIZE	4096	/* ȱʡͨѶת����������С */

#define RULE_ID_MAXLEN				64
#define LOAD_BALANCE_ALGORITHM_MAXLEN		2
#define IP_MAXLEN				30
#define PORT_MAXLEN				10

#define WAIT_EVENTS_COUNT			1024 /* һ�λ�ȡepoll�¼����� */

#define FORWARD_SESSION_TYPE_CLIENT		1
#define FORWARD_SESSION_TYPE_LISTEN		2
#define FORWARD_SESSION_TYPE_SERVER		3

#define FORWARD_SESSION_STATUS_UNUSED		0 /* δʹ�� */
#define FORWARD_SESSION_STATUS_READY		1 /* ׼��ʹ�� */
#define FORWARD_SESSION_STATUS_LISTEN		9 /* ������ */
#define FORWARD_SESSION_STATUS_CONNECTING	11 /* �Ƕ��������� */
#define FORWARD_SESSION_STATUS_CONNECTED	12 /* ������� */

#define IO_BUFFER_SIZE				4096 /* ���������������С */

#define DEFAULT_DISABLE_TIMEOUT			60 /* ȱʡ�ݽ�ʱ�䣬��λ���� */

/* �����ַ��Ϣ�ṹ */
struct NetAddress
{
	char			ip[ IP_MAXLEN + 1 ] ; /* ip��ַ */
	union
	{
		char		port_str[ PORT_MAXLEN + 1 ] ;
		int		port_int ;
	} port ; /* �˿� */
	struct sockaddr_in	sockaddr ; /* sock��ַ�ṹ */
} ;

/* �ͻ�����Ϣ�ṹ */
struct ClientNetAddress
{
	struct NetAddress	netaddr ; /* �����ַ�ṹ */
	
	unsigned long		client_connection_count ; /* �ͻ����������� */
	unsigned long		maxclients ; /* ���ͻ������� */
} ;

/* ת������Ϣ�ṹ */
struct ForwardNetAddress
{
	struct NetAddress	netaddr ; /* �����ַ�ṹ */
	int			sock ; /* sock������ */
} ;

/* �������Ϣ�ṹ */
struct ServerNetAddress
{
	struct NetAddress	netaddr ; /* �����ַ�ṹ */
	
	unsigned char		server_unable ; /* ����˿��ñ�־ */
	time_t			timestamp_to_enable ; /* �ָ�������� */
	
	unsigned long		server_connection_count ; /* ������������� */
	
	struct timeval		tv1 ; /* �����ʱ��� */
	struct timeval		tv2 ; /* ���дʱ��� */
	struct timeval		dtv ; /* �����дʱ����� */
} ;

/* ת������ṹ */
struct ForwardRule
{
	char				rule_id[ RULE_ID_MAXLEN + 1 ] ; /* ����ID � */
	char				load_balance_algorithm[ LOAD_BALANCE_ALGORITHM_MAXLEN + 1 ] ; /* ���ؾ����㷨 */
	
	struct ClientNetAddress		*clients_addr ; /* �ͻ��˵�ַ�ṹ */
	unsigned long			clients_addr_size ; /* �ͻ��˹�������������� */
	unsigned long			clients_addr_count ; /* �ͻ��˹����������� */
	
	struct ForwardNetAddress	*forwards_addr ; /* ת���˵�ַ�ṹ */
	unsigned long			forwards_addr_size ; /* ת���˹�������������� */
	unsigned long			forwards_addr_count ; /* ת���˹����������� */
	
	struct ServerNetAddress		*servers_addr ; /* ����˵�ַ�ṹ */
	unsigned long			servers_addr_size ; /* ����˹�������������� */
	unsigned long			servers_addr_count ; /* ����˹����������� */
	unsigned long			selected_addr_index ; /* ��ǰ��������� */
} ;

/* ת���Ự�ṹ */
struct ForwardSession
{
	unsigned char		status ; /* �Ự״̬ */
	unsigned char		type ; /* �ͻ��˻��Ƿ���� */
	
	struct ForwardRule	*p_forward_rule ; /* ת���������� */
	unsigned long		client_index ; /* �ͻ������� */
	unsigned long		server_index ; /* ��������� */
	
	/*
	union
	{
		struct
		{
			
		} MS ;
	} balance_algorithm ;
	*/
	
	struct NetAddress	netaddr ; /* �����ַ�ṹ */
	int			sock ; /* sock������ */
	
	char			io_buffer[ IO_BUFFER_SIZE + 1 ] ; /* ������������� */
	int			io_buffer_len ; /* ���������������Ч���ݳ��� */
	int			io_buffer_offsetpos ; /* ���������������Ч���ݿ�ʼƫ���� */
	
	struct ForwardSession	*p_reverse_forward_session ; /* ����Ự */
} ;

/* �����в��� */
struct CommandParameter
{
	char			*config_pathfilename ; /* -f ... */
	unsigned long		forward_thread_size ; /* -t ... */
	unsigned long		forward_session_size ; /* -s ... */
	int			log_level ; /* --log-level (DEBUG|INFO|WARN|ERROR|FATAL)*/
	int			no_daemon_flag ; /* --no-daemon (1|0) */
} ;

/* �����������ṹ */
struct PipeFds
{
	int			fds[ 2 ] ;
} ;

struct ServerEnv
{
	struct CommandParameter	cmd_para ; /* �����в����ṹ */
	
	struct ForwardRule	*forward_rules_array ; /* ת���������� */
	unsigned long		forward_rules_size ; /* ת�����������С */
	unsigned long		forward_rules_count ; /* ת��������װ������ */
	
	pid_t			pid ; /* �ӽ���PID */
	struct PipeFds		accept_pipe ; /* ���ӽ�������ܵ� */
	int			accept_epoll_fd ; /* �����˿�epoll�� */
	
	int			*forward_thread_index ; /* ���߳���� */
	pthread_t		*forward_thread_tid_array ; /* ���߳�TID */
	struct PipeFds		*forward_pipe_array ; /* �����߳�����ܵ� */
	int			*forward_epoll_fd_array ; /* ���߳�ת��EPOLL�� */
	
	struct ForwardSession	*forward_session_array ; /* ת���Ự���� */
	unsigned long		forward_session_count ; /* ת���Ự�����С */
	unsigned long		forward_session_use_offsetpos ; /* ת���Ự���ʹ�õ�Ԫƫ���� */
} ;

/********* util *********/

int Rand( int min, int max );
unsigned long CalcHash( char *str );
int SetReuseAddr( int sock );
int SetNonBlocking( int sock );
void SetNetAddress( struct NetAddress *p_netaddr );
void GetNetAddress( struct NetAddress *p_netaddr );
int BindDaemonServer( char *pcServerName , int (* ServerMain)( void *pv ) , void *pv , int (* ControlMain)(long lControlStatus) );
int IsMatchString(char *pcMatchString, char *pcObjectString, char cMatchMuchCharacters, char cMatchOneCharacters);

/********* envirment *********/

int InitEnvirment( struct ServerEnv *penv );
void CleanEnvirment( struct ServerEnv *penv );
int AddListeners( struct ServerEnv *penv );
struct ForwardSession *GetForwardSessionUnused( struct ServerEnv *penv );
void SetForwardSessionUnused( struct ForwardSession *p_forward_session );
void SetForwardSessionUnused2( struct ForwardSession *p_forward_session , struct ForwardSession *p_forward_session2 );

/********* Config *********/

int LoadConfig( struct ServerEnv *penv );
void UnloadConfig( struct ServerEnv *penv );

/********* MonitorProcess *********/

int MonitorProcess( struct ServerEnv *penv );
int _MonitorProcess( void *pv );

/********* WorkerProcess *********/

int WorkerProcess( struct ServerEnv *penv );
int WorkerProcess( struct ServerEnv *penv );

/********* AcceptThread *********/

void *AcceptThread( struct ServerEnv *penv );
void *_AcceptThread( void *pv );

/********* ForwardThread *********/

void *ForwardThread( struct ServerEnv *penv );
void *_ForwardThread( void *pv );

#endif

