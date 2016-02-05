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
#define _VSNPRINTF		vsnprintf
#define _SNPRINTF		snprintf
#define _CLOSESOCKET		close
#define _ERRNO			errno
#define _EWOULDBLOCK		EWOULDBLOCK
#define _ECONNABORTED		ECONNABORTED
#define _EINPROGRESS		EINPROGRESS
#define _ECONNRESET		ECONNRESET
#define _ENOTCONN		ENOTCONN
#define _EISCONN		EISCONN
#define _SOCKLEN_T		socklen_t
#define _GETTIMEOFDAY(_tv_)	gettimeofday(&(_tv_),NULL)
#define _LOCALTIME(_tt_,_stime_) \
	localtime_r(&(_tt_),&(_stime_));
#elif ( defined _WIN32 )
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <io.h>
#include <windows.h>
#define _VSNPRINTF		_vsnprintf
#define _SNPRINTF		_snprintf
#define _CLOSESOCKET		closesocket
#define _ERRNO			GetLastError()
#define _EWOULDBLOCK		WSAEWOULDBLOCK
#define _ECONNABORTED		WSAECONNABORTED
#define _EINPROGRESS		WSAEINPROGRESS
#define _ECONNRESET		WSAECONNRESET
#define _ENOTCONN		WSAENOTCONN
#define _EISCONN		WSAEISCONN
#define _SOCKLEN_T		int
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

#ifndef ULONG_MAX
#define ULONG_MAX 0xffffffffUL
#endif

#define FOUND				9	/* �ҵ� */ /* found */
#define NOT_FOUND			4	/* �Ҳ��� */ /* not found */

#define MATCH				1	/* ƥ�� */ /* match */
#define NOT_MATCH			-1	/* ��ƥ�� */ /* not match */

#define RULE_ID_MAXLEN			64	/* �ת������������ */ /* The longest length of forwarding rules */
#define RULE_MODE_MAXLEN		2	/* �ת������ģʽ���� */ /* The longest length of forwarding rules mode */

#define FORWARD_RULE_MODE_G		"G"	/* ����˿� */ /* manager port */
#define FORWARD_RULE_MODE_MS		"MS"	/* ����ģʽ */ /* master-standby mode */
#define FORWARD_RULE_MODE_RR		"RR"	/* ��ѯģʽ */ /* polling mode */
#define FORWARD_RULE_MODE_LC		"LC"	/* ��������ģʽ */ /* minimum number of connections mode */
#define FORWARD_RULE_MODE_RT		"RT"	/* ��С��Ӧʱ��ģʽ */ /* minimum response time mode */
#define FORWARD_RULE_MODE_RD		"RD"	/* ���ģʽ */ /* random mode */
#define FORWARD_RULE_MODE_HS		"HS"	/* HASHģʽ */ /* HASH mode */

#define RULE_CLIENT_MAXCOUNT		10	/* �������������ͻ����������� */ /* maximum clients in rule */
#define RULE_FORWARD_MAXCOUNT		3	/* �������������ת������������ */ /* maximum forwards in rule */
#define RULE_SERVER_MAXCOUNT		100	/* ������������������������� */ /* maximum servers in rule */

#define DEFAULT_FORWARD_RULE_MAXCOUNT	100	/* ȱʡ���ת���������� */ /* maximum forward rules for default */
#define DEFAULT_CONNECTION_MAXCOUNT	1024	/* ȱʡ����������� */ /* ���ת���Ự���� = ����������� * 3 */ /* maximum connections for default */
#define DEFAULT_TRANSFER_BUFSIZE	4096	/* ȱʡͨѶת����������С */ /* communication I/O buffer for default */




struct ServerEnv
{
	
	
	
	
} ;



#endif

