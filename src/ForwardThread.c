#include "G6.h"

#define DISCONNECT_PAIR	\
	RemoveTimeoutTreeNode2( penv , p_forward_session , p_forward_session->p_reverse_forward_session ); \
	if( p_forward_session->type == FORWARD_SESSION_TYPE_SERVER ) \
	{ \
		RemoveIpConnectionStat( penv , &(penv->ip_connection_stat) , p_forward_session->p_forward_rule->client_addr_array[p_forward_session->client_index].netaddr.sockaddr.sin_addr.s_addr ); \
		SERVER_CONNECTION_COUNT_DECREASE(penv,p_forward_session->p_reverse_forward_session->p_forward_rule->server_addr_array[p_forward_session->p_reverse_forward_session->server_index],1) \
	} \
	else if( p_forward_session->type == FORWARD_SESSION_TYPE_CLIENT ) \
	{ \
		struct ForwardSession	*p_reverse_forward_session = p_forward_session->p_reverse_forward_session ; \
		RemoveIpConnectionStat( penv , &(penv->ip_connection_stat) , p_reverse_forward_session->p_forward_rule->client_addr_array[p_reverse_forward_session->client_index].netaddr.sockaddr.sin_addr.s_addr ); \
		SERVER_CONNECTION_COUNT_DECREASE(penv,p_forward_session->p_forward_rule->server_addr_array[p_forward_session->server_index],1) \
	} \
	epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->sock , NULL ); \
	epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->p_reverse_forward_session->sock , NULL ); \
	DebugLog( __FILE__ , __LINE__ , "close #%d# #%d#" , p_forward_session->sock , p_forward_session->p_reverse_forward_session->sock ); \
	_CLOSESOCKET2( p_forward_session->sock , p_forward_session->p_reverse_forward_session->sock ); \
	SetForwardSessionUnused2( penv , p_forward_session , p_forward_session->p_reverse_forward_session ); \

static void IgnoreReverseSessionEvents( struct ForwardSession *p_forward_session , struct epoll_event *p_events , int event_index , int event_count )
{
	struct epoll_event	*p_event = NULL ;
	
	for( ++event_index , p_event = p_events + event_index ; event_index < event_count ; event_index++ , p_event++ )
	{
		if( p_event->data.ptr == p_forward_session )
			continue;
		
		if( p_event->data.ptr == p_forward_session->p_reverse_forward_session )
			p_event->events = 0 ;
	}
	
	return;
}

static int OnForwardInput( struct ServerEnv *penv , struct ForwardSession *p_forward_session , int forward_epoll_fd , struct epoll_event *p_events , int event_index , int event_count )
{
	int			len ;
	struct epoll_event	event ;
	
	/* ����ͨѶ���� */
	p_forward_session->io_buffer_offsetpos = 0 ;
	p_forward_session->io_buffer_len = recv( p_forward_session->sock , p_forward_session->io_buffer , IO_BUFFER_SIZE , 0 ) ;
	if( p_forward_session->io_buffer_len == 0 )
	{
		/* �Զ˶Ͽ����� */
		InfoLog( __FILE__ , __LINE__ , "recv #%d# closed" , p_forward_session->sock );
		DISCONNECT_PAIR
		return 0;
	}
	else if( p_forward_session->io_buffer_len == -1 )
	{
		/* ͨѶ���ճ��� */
		ErrorLog( __FILE__ , __LINE__ , "recv #%d# failed , errno[%d]" , p_forward_session->sock , errno );
		DISCONNECT_PAIR
		return 0;
	}
	
	/* �Ǽ������ʱ��� */
	pthread_mutex_lock( & (penv->server_connection_count_mutex) );
	if( p_forward_session->type == FORWARD_SESSION_TYPE_CLIENT )
		p_forward_session->p_forward_rule->server_addr_array[p_forward_session->server_index].rtt = GETTIMEVAL.tv_sec ;
	pthread_mutex_unlock( & (penv->server_connection_count_mutex) );
	
	/* ����ͨѶ���� */
	len = send( p_forward_session->p_reverse_forward_session->sock , p_forward_session->io_buffer , p_forward_session->io_buffer_len , 0 ) ;
	if( len == -1 )
	{
		if( _ERRNO == _EWOULDBLOCK )
		{
			/* ͨѶ���ͻ������� */
			IgnoreReverseSessionEvents( p_forward_session , p_events , event_index , event_count );
			
			memset( & event , 0x00 , sizeof(struct epoll_event) );
			event.data.ptr = p_forward_session ;
			event.events = EPOLLERR ;
			epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->sock , & event );
			
			memset( & event , 0x00 , sizeof(struct epoll_event) );
			event.data.ptr = p_forward_session->p_reverse_forward_session ;
			event.events = EPOLLOUT | EPOLLERR ;
			epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->p_reverse_forward_session->sock , & event );
		}
		else
		{
			/* ͨѶ���ͳ��� */
			ErrorLog( __FILE__ , __LINE__ , "send #%d# failed , errno[%d]" , p_forward_session->sock , errno );
			DISCONNECT_PAIR
			return 0;
		}
	}
	else if( len == p_forward_session->io_buffer_len )
	{
		/* һ��ȫ���� */
		InfoLog( __FILE__ , __LINE__ , "transfer #%d# [%d]bytes to #%d#" , p_forward_session->sock , len , p_forward_session->p_reverse_forward_session->sock );
		p_forward_session->io_buffer_len = 0 ;
	}
	else
	{
		/* ֻ�����˲��� */
		InfoLog( __FILE__ , __LINE__ , "transfer #%d# [%d/%d]bytes to #%d#" , p_forward_session->sock , len , p_forward_session->io_buffer_len , p_forward_session->p_reverse_forward_session->sock );
		
		IgnoreReverseSessionEvents( p_forward_session , p_events , event_index , event_count );
		
		p_forward_session->io_buffer_len -= len ;
		p_forward_session->io_buffer_offsetpos = len ;
		
		memset( & event , 0x00 , sizeof(struct epoll_event) );
		event.data.ptr = p_forward_session ;
		event.events = EPOLLERR ;
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->sock , & event );
		
		memset( & event , 0x00 , sizeof(struct epoll_event) );
		event.data.ptr = p_forward_session->p_reverse_forward_session ;
		event.events = EPOLLOUT | EPOLLERR ;
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->p_reverse_forward_session->sock , & event );
	}
	
	/* �Ǽ����дʱ��� */
	pthread_mutex_lock( & (penv->server_connection_count_mutex) );
	if( p_forward_session->type == FORWARD_SESSION_TYPE_CLIENT )
		p_forward_session->p_forward_rule->server_addr_array[p_forward_session->server_index].wtt = GETTIMEVAL.tv_sec ;
	pthread_mutex_unlock( & (penv->server_connection_count_mutex) );
	
	return 0;
}

static int OnForwardOutput( struct ServerEnv *penv , struct ForwardSession *p_forward_session , int forward_epoll_fd , struct epoll_event *p_events , int event_index , int event_count )
{
	int			len ;
	struct epoll_event	event ;
	
	/* ��������ͨѶ���� */
	len = send( p_forward_session->p_reverse_forward_session->sock , p_forward_session->io_buffer + p_forward_session->io_buffer_offsetpos , p_forward_session->io_buffer_len , 0 ) ;
	if( len == -1 )
	{
		if( _ERRNO == _EWOULDBLOCK )
		{
			/* ͨѶ���ͻ������� */
		}
		else
		{
			/* ͨѶ���ͳ��� */
			ErrorLog( __FILE__ , __LINE__ , "send #%d# failed , errno[%d]" , p_forward_session->sock , errno );
			DISCONNECT_PAIR
			return 0;
		}
	}
	else if( len == p_forward_session->io_buffer_len )
	{
		/* һ��ȫ���� */
		InfoLog( __FILE__ , __LINE__ , "transfer #%d# [%d]bytes to #%d#" , p_forward_session->sock , len , p_forward_session->p_reverse_forward_session->sock );
		
		p_forward_session->io_buffer_len = 0 ;
		
		memset( & event , 0x00 , sizeof(struct epoll_event) );
		event.data.ptr = p_forward_session ;
		event.events = EPOLLIN | EPOLLERR ;
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->sock , & event );
		
		memset( & event , 0x00 , sizeof(struct epoll_event) );
		event.data.ptr = p_forward_session->p_reverse_forward_session ;
		event.events = EPOLLIN | EPOLLERR ;
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->p_reverse_forward_session->sock , & event );
	}
	else
	{
		/* ֻ�����˲��� */
		InfoLog( __FILE__ , __LINE__ , "transfer #%d# [%d/%d]bytes to #%d#" , p_forward_session->sock , len , p_forward_session->io_buffer_len , p_forward_session->p_reverse_forward_session->sock );
		
		p_forward_session->io_buffer_len -= len ;
		p_forward_session->io_buffer_offsetpos = len ;
	}
	
	/* �Ǽ����дʱ��� */
	pthread_mutex_lock( & (penv->server_connection_count_mutex) );
	if( p_forward_session->type == FORWARD_SESSION_TYPE_CLIENT )
		p_forward_session->p_forward_rule->server_addr_array[p_forward_session->server_index].wtt = GETTIMEVAL.tv_sec ;
	pthread_mutex_unlock( & (penv->server_connection_count_mutex) );
	
	return 0;
}

void *ForwardThread( unsigned long forward_thread_index )
{
	struct ServerEnv	*penv = g_penv ;
	int			forward_epoll_fd ;
	
	int			timeout ;
	struct epoll_event	events[ WAIT_EVENTS_COUNT ] ;
	int			event_count ;
	int			event_index ;
	struct epoll_event	*p_event = NULL ;
	
	struct ForwardSession	*p_forward_session = NULL ;
	
	int			nret = 0 ;
	
	forward_epoll_fd = penv->forward_epoll_fd_array[forward_thread_index] ;
	
	/* ������־����ļ� */
	SetLogFile( "%s/log/G6.log" , getenv("HOME") );
	SetLogLevel( penv->cmd_para.log_level );
	InfoLog( __FILE__ , __LINE__ , "--- G6.WorkerProcess.ForwardThread.%d ---" , forward_thread_index+1 );
	
	/* ������ѭ�� */
	while( ! ( g_exit_flag == 1 && penv->forward_session_count == 0 ) )
	{
		if( g_exit_flag == 1 )
		{
			timeout = 1000 ;
		}
		else
		{
			timeout = GetLastestTimeout( penv ) ;
		}
		
		DebugLog( __FILE__ , __LINE__ , "epoll_wait [%d][...]... timeout[%d]" , penv->forward_request_pipe[forward_thread_index].fds[0] , timeout );
		CloseLogFile();
		event_count = epoll_wait( forward_epoll_fd , events , WAIT_EVENTS_COUNT , timeout ) ;
		DebugLog( __FILE__ , __LINE__ , "epoll_wait return [%d]events" , event_count );
		
		while( event_count == 0 )
		{
			struct rb_node		*p_node = NULL ;
			struct ForwardSession	*p_forward_session = NULL ;
			
			p_node = rb_first( & (penv->timeout_rbtree) ); 
			if (p_node == NULL)
				break;
			
			p_forward_session = rb_entry( p_node , struct ForwardSession , timeout_rbnode );
			if( p_forward_session->timeout_timestamp - GETTIMEVAL.tv_sec <= 0 )
			{
				ErrorLog( __FILE__ , __LINE__ , "forward session TIMEOUT" );
				DISCONNECT_PAIR
				continue;
			}
			
			break;
		}
		
		for( event_index = 0 , p_event = events ; event_index < event_count ; event_index++ , p_event++ )
		{
			p_forward_session = p_event->data.ptr ;
			
			if( p_forward_session == NULL )
			{
				char		command ;
				
				DebugLog( __FILE__ , __LINE__ , "pipe session event" );
				
				InfoLog( __FILE__ , __LINE__ , "read forward_request_pipe ..." );
				nret = read( penv->forward_request_pipe[forward_thread_index].fds[0] , & command , 1 ) ;
				InfoLog( __FILE__ , __LINE__ , "read forward_request_pipe done[%d][%c]" , nret , command );
				if( nret == -1 )
				{
					ErrorLog( __FILE__ , __LINE__ , "read request pipe failed , errno[%d]" , errno );
					exit(0);
				}
				else if( nret == 0 )
				{
					ErrorLog( __FILE__ , __LINE__ , "read request pipe close" );
					exit(0);
				}
				else
				{
					if( command == 'L' )
					{
						CloseLogFile();
					}
					else if( command == 'Q' )
					{
					}
				}
			}
			else
			{
				DebugLog( __FILE__ , __LINE__ , "forward session event" );
				
				pthread_mutex_lock( & (penv->timeout_rbtree_mutex) );
				p_forward_session->timeout_timestamp = time(NULL) + DEFAULT_ALIVE_TIMEOUT ;
				p_forward_session->p_reverse_forward_session->timeout_timestamp = time(NULL) + DEFAULT_ALIVE_TIMEOUT ;
				pthread_mutex_unlock( & (penv->timeout_rbtree_mutex) );
				
				if( p_event->events & EPOLLIN ) /* �����¼� */
				{
					nret = OnForwardInput( penv , p_forward_session , forward_epoll_fd , events , event_index , event_count ) ;
					if( nret )
					{
						ErrorLog( __FILE__ , __LINE__ , "OnForwardInput failed[%d]" , nret );
					}
				}
				else if( p_event->events & EPOLLOUT ) /* ����¼� */
				{
					nret = OnForwardOutput( penv , p_forward_session , forward_epoll_fd , events , event_index , event_count ) ;
					if( nret )
					{
						ErrorLog( __FILE__ , __LINE__ , "OnForwardOutput failed[%d]" , nret );
					}
				}
				else
				{
					ErrorLog( __FILE__ , __LINE__ , "forward session EPOLLERR" );
					DISCONNECT_PAIR
				}
			}
		}
	}
	
	return NULL;
}

void *_ForwardThread( void *pv )
{
	unsigned int	*p_forward_thread_index = (unsigned int *)pv ;
	unsigned int	forward_thread_index = (*p_forward_thread_index) ;
	
	SETPID
	SETTID
	
	free( p_forward_thread_index );
	ForwardThread( forward_thread_index );
	
	InfoLog( __FILE__ , __LINE__ , "pthread_exit" );
	pthread_exit(NULL);
}

