#include<cstdio>
#include<iostream>
#include "tictactoe.h"

using namespace std;

void Tic_Tac_Toe::calc_result()
{
    if(winner!=0)
        return;
    for(int i=0;i<3;i++)
    {
        if(board[i][0]!=' ' && board[i][0]==board[i][1] && board[i][0]==board[i][2])
            winner= (board[i][0]=='X' ? 1 : 2);
            
        if(board[0][i]!=' ' && board[0][i]==board[1][i] && board[0][i]==board[2][i])
            winner= (board[0][i]=='X' ? 1 : 2) ;
    }
    
    if(board[0][0]!=' ' && board[0][0]==board[1][1] && board[0][0]==board[2][2])
        winner= (board[0][0]=='X' ? 1 : 2);
            
    if(board[0][2]!=' ' && board[0][2]==board[1][1] && board[0][2]==board[2][0])
        winner= (board[0][2]=='X' ? 1 : 2);
        
    if(winner==0 && turn>=9)
    {
        winner=3;
    }
}

bool Tic_Tac_Toe::is_turn(int id)
{
    if(id!=player_ids[turn%2])
    {
        return false;    
    }
    return true;
}
Tic_Tac_Toe::Tic_Tac_Toe(int id1, int id2)
{

    player_ids=new int [2];
    player_ids[0]=id1;
    player_ids[1]=id2;
    
    turn=0;
    winner=0;
    
    board=new char* [3];
    for(int i=0;i<3;i++)
    {
        board[i]=new char[3];
        for(int j=0;j<3;j++)
        {
            board[i][j]=' ';
        }
    }
}
        
Tic_Tac_Toe::~Tic_Tac_Toe()
{
    for(int i=0;i<3;i++)
        delete [] board[i];
            
    delete [] board;
}
        
void Tic_Tac_Toe::next_turn(int row, int col, int id)
{
    if(winner!=0)
        throw "Game already over";
    
    else if(id!=player_ids[turn%2])
    {
        throw "Wrong player";
    }
    
    else if(row<1 || row >3)
    {
        throw "Invalid row index";
    }
    
    else if(col<1 || col >3)
    {
        throw "Invalid col index";
    }
    
    else if(board[row-1][col-1]!=' ')
    {
        throw "position already occupied";
    }
    
    else if((turn++)%2==0)
    {
        board[row-1][col-1]='X';
    }
    else
    {
        board[row-1][col-1]='O';
    }
    this->calc_result();
}

int Tic_Tac_Toe::game_state()
{
    if(winner==0)
        return -1;
    if(winner==1)
        return player_ids[0];
    if(winner==2)
        return player_ids[1];
    if(winner==3)	
        return -2;
        
    return -1;
}

void Tic_Tac_Toe::draw_board(char * str)
{
    
    sprintf(str,"\n%c|%c|%c\n%c|%c|%c\n%c|%c|%c\n\n",
            board[0][0],board[0][1],board[0][2],board[1][0],board[1][1],board[1][2],board[2][0],board[2][1],board[2][2]);
            
}



