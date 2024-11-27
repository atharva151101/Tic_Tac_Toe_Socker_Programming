#include<iostream>


#ifndef TIC_TAC_TOE
#define TIC_TAC_TOE
class Tic_Tac_Toe{
    
    private:
        char ** board;
        int * player_ids;
        int turn;
        void calc_result();      
        int winner;  
        
    public:
        Tic_Tac_Toe(int id1, int id2);
        ~Tic_Tac_Toe();
        bool is_turn(int id);
        void next_turn(int row, int col, int id);
        int game_state();
        void draw_board(char *);
   
};
#endif

