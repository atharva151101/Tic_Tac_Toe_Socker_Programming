#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <sstream>
#include <errno.h>
#include <chrono>
#include <fstream>
#include "tictactoe.h"
#include<signal.h>
 
#define PORT_NO 3023
#define MAX_QUEUE 1
#define MAX_GAMES 100
#define TIME_OUT 30

using namespace std;
using namespace std::chrono;

high_resolution_clock::time_point server_start;
int player_id;        
int game_id;          
int socket_fd;
ofstream fout;

Tic_Tac_Toe * Games[MAX_GAMES];
pthread_mutex_t print_lock;
pthread_mutex_t game_locks[MAX_GAMES];
pthread_t threads[2*MAX_GAMES];
bool game_over[MAX_GAMES];
bool time_out[MAX_GAMES];
bool has_disconnected[MAX_GAMES];

struct client
{
    int connect_fd[2];
    int player_id;
    int game_id;
    int play_again;
};

struct thread_argument
{   
    struct client * clients;
    int cli_no;
    Tic_Tac_Toe * Game;
};

int start_server();
void connect_two_clients(struct client * clients,int socket_fd);
void initialize_game(struct client * clients);
void play_again(struct client * clients, int cli_no,int ans);
void result(struct client * clients, int cli_no, int result);
void * run_game(void * thread_data);

double current_time()
{
    auto curr=high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(curr - server_start);
    
    return (double)duration.count()/1000;
}

void interrupt_handler(int temp)
{

    close(socket_fd);
    fout.close();
    exit(1);
}


int start_server()
{
     
    socket_fd = socket (AF_INET, SOCK_STREAM, 0);
    if(socket_fd<0)
    {
        perror("Could not initialize the socket");
        exit(-1);
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT_NO);

    
    if(bind(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))<0)
    {
        perror("Error : Could not bind address to the socket.");
        exit(-1);
    }
    
    return socket_fd;
} 

void connect_two_clients(struct client * clients,int socket_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_size = sizeof(client_addr);
    
    clients[0].connect_fd[0] = accept(socket_fd, (struct sockaddr *) &client_addr, &client_size);
    clients[0].connect_fd[1] = accept(socket_fd, (struct sockaddr *) &client_addr, &client_size);
    clients[0].player_id=player_id++;
    
    pthread_mutex_lock(&print_lock);
    fout<<"<EVENT : New Client Connected  ,  CLIENT_ID : "<<clients[0].player_id<<"  ,  TIME : "<<current_time()<<">"<<endl<<flush;
    pthread_mutex_unlock(&print_lock);
    
    char buffer[1000];
    sprintf(buffer, "Connected to the game server. Your player ID is %d\n",clients[0].player_id);
    send(clients[0].connect_fd[0], buffer, 1000,0);
    
    
    sprintf(buffer, "Waiting for a partner to join .....\n");
    send(clients[0].connect_fd[0], buffer, 1000,0);
    
    
    clients[1].connect_fd[0] = accept(socket_fd, (struct sockaddr *) &client_addr, &client_size);
    clients[1].connect_fd[1] = accept(socket_fd, (struct sockaddr *) &client_addr, &client_size);
    clients[1].player_id=player_id++;
    
    
    pthread_mutex_lock(&print_lock);
    fout<<"<EVENT : New Client Connected  ,  CLIENT_ID : "<<clients[1].player_id<<"  ,  TIME : "<<current_time()<<">"<<endl<<flush;
    pthread_mutex_unlock(&print_lock);
    
    
    sprintf(buffer, "Connected to the game server. Your player ID is %d\n",clients[1].player_id);
    send(clients[1].connect_fd[0], buffer, 1000,0);
    
    sprintf(buffer, "\rPartner has joined. Your partners ID is %d.\n",clients[1].player_id);
    send(clients[0].connect_fd[0], buffer, 1000,0);
    
    sprintf(buffer, "\rPartner has joined. Your partners ID is %d.\n",clients[0].player_id);
    send(clients[1].connect_fd[0], buffer, 1000,0);
    
}


void initialize_game(struct client * clients)
{
    clients[0].game_id=game_id;
    clients[0].play_again=0;
    clients[1].game_id=game_id;
    clients[1].play_again=0;
    
    game_over[game_id]=false;
    time_out[game_id]=false;
    has_disconnected[game_id]=false;
    
    char buffer[1000];
    sprintf(buffer, "\n\nThe Game has started. The Game ID of this game is %d\n",game_id);
    send(clients[0].connect_fd[0], buffer, 1000,0);
    send(clients[1].connect_fd[0], buffer, 1000,0);
    
    int random=rand()%2;
    sprintf(buffer, "Your symbol is 'X'. You will play first\n\n");
    send(clients[random].connect_fd[0], buffer, 1000,0);
    sprintf(buffer, "Your symbol is 'O'. Your partner will play first\n\n");
    send(clients[1-random].connect_fd[0], buffer, 1000,0);
        
    pthread_mutex_lock(&print_lock);
    fout<<"<EVENT : New Game Starting  ,  GAME_ID : "<<game_id<<"  ,  PLAYER1_ID : "<<clients[random].player_id<<"  ,  PLAYER2_ID : "<<clients[1-random].player_id<<"  ,  TIME : "<<current_time()<<">"<<endl<<flush;
    pthread_mutex_unlock(&print_lock);
    
    
    Games[game_id]= new Tic_Tac_Toe(clients[random].player_id,clients[1-random].player_id);
    struct thread_argument * argument1=new thread_argument; 
    argument1->clients=clients; argument1->cli_no=random; argument1->Game=Games[game_id];
    struct thread_argument * argument2=new thread_argument; 
    argument2->clients=clients; argument2->cli_no=1-random; argument2->Game=Games[game_id];
    
    pthread_mutex_init(&game_locks[game_id], NULL);    
    pthread_create(&threads[game_id*2],NULL,run_game,(void *)argument1);
    pthread_create(&threads[game_id*2+1],NULL,run_game	,(void *)argument2);
    game_id++;
    
    return;
}


void play_again(struct client * clients, int cli_no)
{
    char buffer[1000];
    sprintf(buffer,"Do you want to play again? Yes/No : ");
    send(clients[cli_no].connect_fd[0],buffer,1000,0);
    
    int ans;
    while(1)
    {
        char buffer2[1000];
        auto start=high_resolution_clock::now();
        bool recieved=false;
        while(1)
        {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(clients[cli_no].connect_fd[1], &fds);
            
            struct timeval tv = {0, 1000};
            int ret=select(clients[cli_no].connect_fd[1]+1,&fds,NULL,NULL,&tv);
            if(ret<=0)
            {  
                auto stop=high_resolution_clock::now();
                auto duration = duration_cast<microseconds>(stop - start);
                if(duration.count()>TIME_OUT*1000000)
                {
                    sprintf(buffer,"\nDidn't recieve an answer. Thanks for playing . Disconnecting from the server...\n");
                    send(clients[cli_no].connect_fd[0],buffer,1000,0);
                    ans=2;
                    shutdown(clients[cli_no].connect_fd[0],SHUT_RDWR);
                    shutdown(clients[cli_no].connect_fd[1],SHUT_RDWR);
                    close(clients[cli_no].connect_fd[0]);
                    close(clients[cli_no].connect_fd[1]);
                    pthread_mutex_lock(&print_lock);
                    fout<<"<EVENT : Client Disconnected  ,  CLIENT_ID : "<<clients[cli_no].player_id<<"  ,  TIME : "<<current_time()<<">"<<endl<<flush;
                    pthread_mutex_unlock(&print_lock);
    
                    break;
                }
                    
            }
            else if(FD_ISSET(clients[cli_no].connect_fd[1],&fds))
            {   
                recieved=true;
                break;
            }
        }
        if(recieved)    
        {
            int len=recv(clients[cli_no].connect_fd[1], buffer2, 1000,0);
            string str(buffer2);
            if(len==0)
            {
                ans=2;
                shutdown(clients[cli_no].connect_fd[0],SHUT_RDWR);
                shutdown(clients[cli_no].connect_fd[1],SHUT_RDWR);
                close(clients[cli_no].connect_fd[0]);
                close(clients[cli_no].connect_fd[1]);
                pthread_mutex_lock(&print_lock);
                fout<<"<EVENT : Client Disconnected  ,  CLIENT_ID : "<<clients[cli_no].player_id<<"  ,  TIME : "<<current_time()<<">"<<endl<<flush;
                pthread_mutex_unlock(&print_lock);
    
                break;
            }
            else if(str.find("Yes")!=string::npos)
            {
                ans=1;
                sprintf(buffer,"Waiting for your partner to answer....\n");
                send(clients[cli_no].connect_fd[0],buffer,1000,0);
                break;   
            }
            else if(str.find("No")!=string::npos)
            {   
                ans=2;
                sprintf(buffer,"Thanks for playing . Disconnecting from the server...\n");
                send(clients[cli_no].connect_fd[0],buffer,1000,0);
                shutdown(clients[cli_no].connect_fd[0],SHUT_RDWR);
                shutdown(clients[cli_no].connect_fd[1],SHUT_RDWR);
                close(clients[cli_no].connect_fd[0]);
                close(clients[cli_no].connect_fd[1]);
                pthread_mutex_lock(&print_lock);
                fout<<"<EVENT : Client Disconnected  ,  CLIENT_ID : "<<clients[cli_no].player_id<<"  ,  TIME : "<<current_time()<<">"<<endl<<flush;
                pthread_mutex_unlock(&print_lock);
    
                break;
            }
            else
            {
                sprintf(buffer,"Please Enter Yes/No : ");
                send(clients[cli_no].connect_fd[0],buffer,1000,0);
                continue;
            }
        }
        else
            break;
    }
    
    if(cli_no==1)
    {
        clients[cli_no].play_again=ans;
    }
    else if(cli_no==0)
    {
        while(1)
        {
            struct timeval tv = {0, 100};
            select(0,NULL,NULL,NULL,&tv);
            if(clients[1].play_again==1 && ans==1)
            {
                    initialize_game(clients);
            }
            else if(clients[1].play_again==2 && ans==1)
            {
                sprintf(buffer,"Sorry, Your partner has disconnected hence new game cannot be played. Disconnecting from the server.... \n");
                send(clients[cli_no].connect_fd[0],buffer,1000,0);
                shutdown(clients[cli_no].connect_fd[0],SHUT_RDWR);
                shutdown(clients[cli_no].connect_fd[1],SHUT_RDWR);
                close(clients[cli_no].connect_fd[0]);
                close(clients[cli_no].connect_fd[1]);
                pthread_mutex_lock(&print_lock);
                fout<<"<EVENT : Client Disconnected  ,  CLIENT_ID : "<<clients[cli_no].player_id<<"  ,  TIME : "<<current_time()<<">"<<endl<<flush;
                pthread_mutex_unlock(&print_lock);
    
                break;
            }
            else if(clients[1].play_again==1 && ans==2)
            {
                sprintf(buffer,"Sorry, Your partner has disconnected hence new game cannot be played. Disconnecting from the server.... \n");
                send(clients[1-cli_no].connect_fd[0],buffer,1000,0);
                shutdown(clients[1-cli_no].connect_fd[0],SHUT_RDWR);
                shutdown(clients[1-cli_no].connect_fd[1],SHUT_RDWR);
                close(clients[1-cli_no].connect_fd[0]);
                close(clients[1-cli_no].connect_fd[1]);
                pthread_mutex_lock(&print_lock);
                fout<<"<EVENT : Client Disconnected  ,  CLIENT_ID : "<<clients[1-cli_no].player_id<<"  ,  TIME : "<<current_time()<<">"<<endl<<flush;
                pthread_mutex_unlock(&print_lock);
    
                break;
            }
        }
    }
    pthread_exit(NULL);
}

void result(struct client * clients, int cli_no, int result)
{
    char buffer[1000];
    if(result==-2)
    {
        sprintf(buffer,"It's a Draw !\n\n");
        
    }
    else if(result==clients[cli_no].player_id)
    {
        sprintf(buffer,"Congrats! You Won! :)\n\n");
    }
    else
    {
        sprintf(buffer,"Sorry, You Lost :(\n\n");
    }
    
    send(clients[cli_no].connect_fd[0],buffer,1000,0);
    
    pthread_mutex_lock(&print_lock);
    if(cli_no==1)
    {
        if(result==-2)
            fout<<"<EVENT : Game Ended  ,  GAME_ID : "<<clients[cli_no].game_id<<"  ,  Reason : Game Was Completed  ,  Result : Draw"<<"  ,  TIME : "<<current_time()<<">"<<endl<<flush;
        else
            fout<<"<EVENT : Game Ended  ,  GAME_ID : "<<clients[cli_no].game_id<<"  ,  Reason : Game Was Completed  ,  Result : Player "<<result<<" Won"<<"  ,  TIME : "<<current_time()<<">"<<endl<<flush;
        
    }
    pthread_mutex_unlock(&print_lock);
    play_again(clients,cli_no);
    
    
}

void * run_game(void * thread_data)
{
    
    struct thread_argument * arg = (struct thread_argument *) thread_data;
    struct client cli=arg->clients[arg->cli_no];
    Tic_Tac_Toe * game=arg->Game;
    int game_id=cli.game_id;
    
    char buffer[1000];
    pthread_mutex_lock(&game_locks[cli.game_id]);
      
    game->draw_board(buffer);
    send(cli.connect_fd[0],buffer,1000,0);
    sprintf(buffer,"Your turn to play! Place an 'X'\nEnter (Row  Col) for placing your mark : ");
    if(game->is_turn(cli.player_id))
        send(cli.connect_fd[0],buffer,1000,0);
    pthread_mutex_unlock(&game_locks[cli.game_id]);
    
    
    while(1)
    {
        while(1)
        {
            auto start=high_resolution_clock::now();
            bool recieved=false;
            while(1)
            {
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(cli.connect_fd[1], &fds);
            
                struct timeval tv = {0, 1000};
                int ret=select(cli.connect_fd[1]+1,&fds,NULL,NULL,&tv);
                if(ret<=0)
                {   
                    if(has_disconnected[cli.game_id])
                    {
                        sprintf(buffer,"Sorry, your partner has disconnected from the server. Closing the game and disconnecting you from the server. \n");
                        send(cli.connect_fd[0],buffer,1000,0);
                        shutdown(cli.connect_fd[0],SHUT_RDWR);
                        shutdown(cli.connect_fd[1],SHUT_RDWR);
                        close(cli.connect_fd[0]);
                        close(cli.connect_fd[1]);
                        pthread_mutex_lock(&print_lock);
                        fout<<"<EVENT : Client Disconnected  ,  CLIENT_ID : "<<cli.player_id<<"  ,  TIME : "<<current_time()<<">"<<endl<<flush;
                        fout<<"<EVENT : Game Ended  ,  GAME_ID : "<<cli.game_id<<"  ,  Reason : Player "<<arg->clients[1-arg->cli_no].player_id<<" Disconnected  ,  Result : NA"<<"  ,  TIME : "<<current_time()<<">"<<endl<<flush;
                        pthread_mutex_unlock(&print_lock);
    
                        pthread_exit(NULL);
                    }
                    else if(game_over[cli.game_id])
                    {   
                        if(!time_out[cli.game_id])
                            result(arg->clients, arg->cli_no, game->game_state());
                        else
                        {
                            sprintf(buffer,"\nYour partner didn't make a move for long time. The game has ended due to inactivity. You may play a new game.\n");
                            send(cli.connect_fd[0],buffer,1000,0);
                            play_again(arg->clients,arg->cli_no);
                        }
                    }
                    else if(!game->is_turn(cli.player_id))
                        break;
                    else
                    {
                        auto stop=high_resolution_clock::now();
                        auto duration = duration_cast<microseconds>(stop - start);
                        if(duration.count()>TIME_OUT*1000000)
                        {
                            sprintf(buffer,"\nYou didn't make a move for a long time. The game has ended due to inactivity. You may play a new game.\n");
                            time_out[cli.game_id]=true;
                            game_over[cli.game_id]=true;
                            send(cli.connect_fd[0],buffer,1000,0);
                            pthread_mutex_lock(&print_lock);
                            fout<<"<EVENT : Game Ended  ,  GAME_ID : "<<cli.game_id<<"  ,  Reason : Player "<<cli.player_id<<" Became Inactive  ,  Result : NA"<<"  ,  TIME : "<<current_time()<<">"<<endl<<flush;
                            pthread_mutex_unlock(&print_lock);
                            play_again(arg->clients,arg->cli_no);
                        }
                    }
                    
            
                }
                else if(FD_ISSET(cli.connect_fd[1],&fds))
                {
                    int len=recv(cli.connect_fd[1], buffer, 1000,0);
                    if(len==0)
                    {   
                        has_disconnected[cli.game_id]=true;
                        game_over[cli.game_id]=true;
                        shutdown(cli.connect_fd[0],SHUT_RDWR);
                        shutdown(cli.connect_fd[1],SHUT_RDWR);
                        close(cli.connect_fd[0]);
                        close(cli.connect_fd[1]);
                        pthread_mutex_lock(&print_lock);
                        fout<<"<EVENT : Client Disconnected  ,  CLIENT_ID : "<<cli.player_id<<"  ,  TIME : "<<current_time()<<">"<<endl<<flush;
                        pthread_mutex_unlock(&print_lock);
    
                        pthread_exit(NULL);
                    }
                    recieved=true;
                    break;
                }
            }
            if(recieved)
                break;
        }
        
        pthread_mutex_lock(&game_locks[cli.game_id]);
        stringstream buff(buffer);
        int row,col;
        buff>>row>>col;
        try {game->next_turn(row,col,cli.player_id);}
        catch (char const* exp)
        {
            string s(exp);
            if(s.compare("Wrong player")==0)
            {
                sprintf(buffer,"Not your turn. Please wait for your partner to play.\n");
                send(cli.connect_fd[0],buffer,1000,0);
            }
            else if(s.compare("Invalid row index")==0)
            {
                sprintf(buffer,"Invalid Row no entered. Row no should be between 1 and 3.\nEnter (Row  Col) for placing your mark : ");
                send(cli.connect_fd[0],buffer,1000,0);
            }
            else if(s.compare("Invalid col index")==0)
            {
                sprintf(buffer,"Invalid col no entered. Row no should be between 1 and 3.\nEnter (Row  Col) for placing your mark : ");
                send(cli.connect_fd[0],buffer,1000,0);
            }
            else if(s.compare("position already occupied")==0)
            {
                sprintf(buffer,"That position has already been occupied. Please choose a different position.\nEnter (Row  Col) for placing your mark : ");
                send(cli.connect_fd[0],buffer,1000,0);
            }
            pthread_mutex_unlock(&game_locks[cli.game_id]);
            continue;
        }
        if(game->game_state()!=-1)
        {   
            sprintf(buffer,"\n");
            send(arg->clients[arg->cli_no].connect_fd[0],buffer,1000,0);
            send(arg->clients[1-arg->cli_no].connect_fd[0],buffer,1000,0);
            game->draw_board(buffer);
            send(arg->clients[arg->cli_no].connect_fd[0],buffer,1000,0);
            send(arg->clients[1-arg->cli_no].connect_fd[0],buffer,1000,0);
            game_over[cli.game_id]=true;
            pthread_mutex_unlock(&game_locks[cli.game_id]);
    
            pthread_mutex_lock(&print_lock);
            fout<<"<EVENT : Move By Client  ,  GAME_ID : "<<cli.game_id<<",  CLIENT_ID : "<<cli.player_id<<"  ,  ROW_ID : "<<row<<"  ,  COL_ID : "<<col<<"  ,  TIME : "<<current_time()<<">"<<endl<<flush;
            pthread_mutex_unlock(&print_lock);
    
    
            result(arg->clients, arg->cli_no, game->game_state());
            
        }
        
        game->draw_board(buffer);
        send(arg->clients[arg->cli_no].connect_fd[0],buffer,1000,0);
        send(arg->clients[1-arg->cli_no].connect_fd[0],buffer,1000,0);
        pthread_mutex_lock(&print_lock);
        fout<<"<EVENT : Move By Client  ,  GAME_ID : "<<cli.game_id<<",  CLIENT_ID : "<<cli.player_id<<"  ,  ROW_ID : "<<row<<"  ,  COL_ID : "<<col<<"  ,  TIME : "<<current_time()<<">"<<endl<<flush;
        pthread_mutex_unlock(&print_lock);
    
        sprintf(buffer,"Your partner's turn.\n");
        send(arg->clients[arg->cli_no].connect_fd[0],buffer,1000,0);
        sprintf(buffer,"Your turn to play! Place an %c\nEnter (Row  Col) for placing your mark : ", arg->cli_no==1?'X':'O');
        send(arg->clients[1-arg->cli_no].connect_fd[0],buffer,1000,0);
        
        pthread_mutex_unlock(&game_locks[cli.game_id]);
    }
    
}

int main (int argc, char **argv)
{   
    srand(time(0)); 
    
    player_id=1;
    game_id=1;
    
    fout.open("log.txt");
    fout<<"<EVENT : Server Is Online  ,  TIME : 0.0>"<<endl<<flush;
    server_start = high_resolution_clock::now();
    int socket_fd=start_server();
    signal(SIGINT, interrupt_handler);
    
    listen(socket_fd, MAX_QUEUE);	
    pthread_mutex_init(&print_lock, NULL); 
	
    for (int i=0; i<MAX_GAMES;i++) 
    {
        struct client * clients=(struct client *)malloc(2*sizeof(struct client));
  
        connect_two_clients(clients,socket_fd);
        initialize_game(clients);
    }
    
    for(int i=0;i<MAX_GAMES;i++)
    {
        pthread_join(threads[i*2],NULL);
        pthread_join(threads[i*2+1],NULL);
    }
    fout.close();
    close(socket_fd); 
}

