#include "G6.h"

/* ȡ��������ߺ��� */
int Rand( int min, int max )
{
	return ( rand() % ( max - min + 1 ) ) + min ;
}

/* �����ַ���HASH���ߺ��� */
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

/* ����sock����ѡ�� */
int SetReuseAddr( int sock )
{
	int	on ;
	
	on = 1 ;
	setsockopt( sock , SOL_SOCKET , SO_REUSEADDR , (void *) & on, sizeof(on) );
	
	return 0;
}

/* ����sock�Ƕ���ѡ�� */
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

/* ��IP��PORT����sockaddr�ṹ */
void SetNetAddress( struct NetAddress *p_netaddr )
{
	memset( & (p_netaddr->sockaddr) , 0x00 , sizeof(struct sockaddr_in) );
	p_netaddr->sockaddr.sin_family = AF_INET ;
	p_netaddr->sockaddr.sin_addr.s_addr = inet_addr(p_netaddr->ip) ;
	p_netaddr->sockaddr.sin_port = htons( (unsigned short)p_netaddr->port.port_int );
	return;
}

/* ��sockaddr�ṹ����IP��PORT */
void GetNetAddress( struct NetAddress *p_netaddr )
{
	strcpy( p_netaddr->ip , inet_ntoa(p_netaddr->sockaddr.sin_addr) );
	p_netaddr->port.port_int = (int)ntohs( p_netaddr->sockaddr.sin_port ) ;
	return;
}

/* ת��Ϊ�ػ����� */
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
