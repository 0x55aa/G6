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

