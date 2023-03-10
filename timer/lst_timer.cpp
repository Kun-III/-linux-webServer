#include "lst_timer.h"
#include "http_conn.h"
sort_timer_lst::sort_timer_lst()
{
    head = NULL;
    tail = NULL;
}

sort_timer_lst::~sort_timer_lst()
{
    until_timer *tmp = head;
    while (tmp)
    {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(until_timer *timer)
{
    if (!timer)
        return;
    if (head == NULL)
        head = tail = timer;
    if (timer->experc < head->experc)
    {
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    add_timer(timer, head);
}

void sort_timer_lst::adjust_timer(until_timer *timer)
{
    printf("446\n");
    if (!timer)
        return;
    printf("447\n");
    until_timer *tmp = timer->next;
    if (timer->experc < tmp->experc)
    {
        return;
    }
    if (timer == head)
    {
        printf("456\n");
        head = head->next;
        timer->prev = NULL;
        timer->next = NULL;
        add_timer(timer, head);
    }
    else
    {
        printf("457\n");
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;

        add_timer(timer, timer->next);
    }
}

void sort_timer_lst::del_timer(until_timer *timer)
{
    if (!timer)
        return;
    if ((timer == head) && (timer == tail))
    {
        head = tail = NULL;
        delete timer;
        return;
    }
    if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        delete timer;
        return;
    }
    if (timer == tail)
    {
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
    return;
}

void sort_timer_lst::tick()
{
    if (!head)
        return;
    time_t cur = time(NULL);
    until_timer *tmp = head;
    while (tmp)
    {
        if (cur < tmp->experc)
            break;
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if (head)
            head->prev = NULL;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(until_timer *timer, until_timer *lst_head)
{
    until_timer *prev = lst_head;
    until_timer *tmp = lst_head->next;
    while (tmp)
    {
        if (timer->experc < tmp->experc)
        {
            prev->next = timer;
            timer->next = tmp;

            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    if (!tmp)
    {
        prev->next = timer;
        timer->prev = prev;
        timer->next = tmp;
        tail = timer;
    }
}

void Utils::init(int timeslot)
{
    m_TIEMSHOT = timeslot;
}

// 设置为非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;
    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

// 设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
}

// 将信号编号写入到管道中，通知主循环有信号需要处理
void Utils::sig_handler(int sig)
{
    int save_err = errno;
    int msg = sig;
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_err;
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data *userdata)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, userdata->socfd, 0);
    close(userdata->socfd);
    http_conn::m_user_count--;
}