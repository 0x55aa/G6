#include "G6.h"

/* 取随机数工具函数 */
int Rand( int min, int max )
{
	return ( rand() % ( max - min + 1 ) ) + min ;
}

/* 计算字符串HASH工具函数 */
unsigned long CalcHash( char *str )
{
	unsigned long	hashval ;
	unsigned char	*puc = NULL ;
	
	hashval = 19791007 ;
	for( puc = (unsigned char *)str ; *puc ; puc++ )
	{
		hashval = hashval * 19830923 + (*puc) ;
	}
	
	return hashval;
}

/* 设置sock重用选项 */
int SetReuseAddr( int sock )
{
	int	on ;
	
	on = 1 ;
	setsockopt( sock , SOL_SOCKET , SO_REUSEADDR , (void *) & on, sizeof(on) );
	
	return 0;
}

/* 设置sock非堵塞选项 */
int SetNonBlocking( int sock )
{
#if ( defined __linux ) || ( defined __unix )
	int	opts;
	
	opts = fcntl( sock , F_GETFL ) ;
	if( opts < 0 )
	{
		return -1;
	}
	
	opts = opts | O_NONBLOCK;
	if( fcntl( sock , F_SETFL , opts ) < 0 )
	{
		return -2;
	}
#elif ( defined _WIN32 )
	u_long	mode = 1 ;
	ioctlsocket( sock , FIONBIO , & mode );
#endif
	
	return 0;
}

void SetNetAddress( struct NetAddress *p_netaddr )
{
	memset( & (p_netaddr->sockaddr) , 0x00 , sizeof(struct sockaddr_in) );
	p_netaddr->sockaddr.sin_family = AF_INET ;
	p_netaddr->sockaddr.sin_addr.s_addr = inet_addr( p_netaddr->ip ) ;
	p_netaddr->sockaddr.sin_port = htons( (unsigned short)p_netaddr->port.port_int );
	return;
}

void GetNetAddress( struct NetAddress *p_netaddr )
{
	strcpy( p_netaddr->ip , inet_ntoa( p_netaddr->sockaddr.sin_addr ) );
	p_netaddr->port.port_int = (int)ntohs( p_netaddr->sockaddr.sin_port ) ;
	return;
}

/* 转换为守护进程 */
int BindDaemonServer( char *pcServerName , int (* ServerMain)( void *pv ) , void *pv , int (* ControlMain)(long lControlStatus) )
{
	int pid;
	int sig,fd;
	
	pid=fork();
	switch( pid )
	{
		case -1:
			return -1;
		case 0:
			break;
		default		:
			exit( 0 );	
			break;
	}

	setsid() ;
	signal( SIGHUP,SIG_IGN );

	pid=fork();
	switch( pid )
	{
		case -1:
			return -2;
		case 0:
			break ;
		default:
			exit( 0 );
			break;
	}
	
	setuid( getpid() ) ;
	
	chdir("/tmp");
	
	umask( 0 ) ;
	
	for( sig=0 ; sig<30 ; sig++ )
		signal( sig , SIG_IGN );
	
	for( fd=0 ; fd<=2 ; fd++ )
		close( fd );
	
	return ServerMain( pv );
}
