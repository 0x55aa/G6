#include "G6.h"

int WorkerProcess( struct ServerEnv *penv )
{
	unsigned int		forward_thread_index ;
	unsigned int		*p_forward_thread_index = NULL ;
	
	int			nret = 0 ;
	
	/* ������־����ļ� */
	InfoLog( __FILE__ , __LINE__ , "--- G6.WorkerProcess ---" );
	
	signal( SIGTERM , SIG_IGN );
	signal( SIGUSR1 , SIG_IGN );
	signal( SIGUSR2 , SIG_IGN );
	g_penv = penv ;
	
	/* ����ת�����߳� */
	for( forward_thread_index = 0 ; forward_thread_index < penv->cmd_para.forward_thread_size ; forward_thread_index++ )
	{
		p_forward_thread_index = (unsigned int *)malloc( sizeof(unsigned int) ) ;
		if( p_forward_thread_index == NULL )
		{
			ErrorLog( __FILE__ , __LINE__ , "malloc failed , errno[%d]" , errno );
			exit(9);
		}
		(*p_forward_thread_index) = forward_thread_index ;
		
		/* ���������շ��߳� */
		nret = pthread_create( penv->forward_thread_tid_array+forward_thread_index , NULL , & _ForwardThread , (void*)p_forward_thread_index ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "pthread_create forward thread failed , errno[%d]" , errno );
			return -1;
		}
		else
		{
			InfoLog( __FILE__ , __LINE__ , "parent_thread : [%lu] pthread_create [%lu]" , pthread_self() , penv->forward_thread_tid_array[forward_thread_index] );
		}
	}
	
	/* ���߳�Ϊ���ӽ����߳� */
	_AcceptThread( (void*)penv );
	
	/* �����˳������ַ������������շ��߳� */
	for( forward_thread_index = 0 ; forward_thread_index < penv->cmd_para.forward_thread_size ; forward_thread_index++ )
	{
		InfoLog( __FILE__ , __LINE__ , "write forward_request_pipe Q ..." );
		nret = write( penv->forward_request_pipe[forward_thread_index].fds[1] , "Q" , 1 ) ;
		InfoLog( __FILE__ , __LINE__ , "write forward_request_pipe Q done[%d]" , nret );
		
		InfoLog( __FILE__ , __LINE__ , "parent_thread : [%lu] pthread_join [%lu] ..." , pthread_self() , penv->forward_thread_tid_array[forward_thread_index] );
		pthread_join( penv->forward_thread_tid_array[forward_thread_index] , NULL );
		InfoLog( __FILE__ , __LINE__ , "parent_thread : [%lu] pthread_join [%lu] ok" , pthread_self() , penv->forward_thread_tid_array[forward_thread_index] );
	}
	
	return 0;
}

