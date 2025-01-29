#include "global.h"
#include "board.h"
#include "move.h"
#include "comm.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

/**********************************************************/
Position gamePosition;		// Position we are going to use

Move moveReceived;			// temporary move to retrieve opponent's choice
Move myMove;				// move to save our choice and send it to the server

char myColor;				// to store our color
int mySocket;				// our socket
char msg;					// used to store the received message

char * agentName = "dker0028";		//default name.. change it! keep in mind MAX_NAME_LENGTH

char * ip = "127.0.0.1";	// default ip (local machine)
/**********************************************************/

int prune = 0;
/*
 * Random player 
 * - not the most efficient implementation
 */
void playRandom( void )
{
	int i,j;

	while( 1 )
	{
		i = rand() % ARRAY_BOARD_SIZE;
		j = rand() % ARRAY_BOARD_SIZE;

		if( gamePosition.board[ i ][ j ] == EMPTY )
		{
			myMove.tile[ 0 ] = i;
			myMove.tile[ 1 ] = j;
			if( isLegalMove( &gamePosition, &myMove ) )
				break;
		}
	}
}

int evaluatePosition(Position *pos, char color,
					 int difweight, int mobilityweight, int stabilityweight) {
	int score = 0;
    int opponent = getOtherSide(color);

    // Basic score difference
    score += (pos->score[color] - pos->score[opponent]) * difweight;

    // Mobility
    int myMobility = 0;
    int opponentMobility = 0;
    for (int i = 0; i < ARRAY_BOARD_SIZE; i++) {
        for (int j = 0; j < ARRAY_BOARD_SIZE; j++) {
            if (isLegal(pos, i, j, color)) { myMobility++; }
            if (isLegal(pos, i, j, opponent)) { opponentMobility++; }
        }
    } score += (myMobility - opponentMobility) * mobilityweight;

    // Stability (corners and edges)
    int stability = 0;
	int c = 10, e = 5, m = 3; // Weights for diferent positions in the board
    int stabilityWeights[ARRAY_BOARD_SIZE][ARRAY_BOARD_SIZE] = {
        {  0,  0,  0,  0,  0,  0,  0,  c,  e,  e,  e,  e,  e,  e,  c },
        {  0,  0,  0,  0,  0,  0,  e, -c, -e, -e, -e, -e, -e, -c,  5 },
        {  0,  0,  0,  0,  0,  e, -e,  m,  m,  m,  m,  m,  m, -e,  5 },
        {  0,  0,  0,  0,  e, -e,  m,  m,  m,  m,  m,  m,  m, -e,  5 },
        {  0,  0,  0,  e, -e,  m,  m,  0,  m,  m,  m,  m,  m, -e,  5 },
        {  0,  0,  e, -e,  m,  m,  0,  0,  0,  m,  m,  m,  m, -e,  5 },
        {  0,  e, -e,  m,  m,  0,  0,  0,  0,  0,  m,  m,  m, -e,  5 },
        {  c, -c,  m,  m,  0,  0,  0,  0,  0,  0,  0,  m,  m, -c,  c },
        {  e, -e,  m,  m,  m,  0,  0,  0,  0,  0,  m,  m, -e,  e,  0 },
        {  e, -e,  m,  m,  m,  m,  0,  0,  0,  m,  m, -e,  e,  0,  0 },
        {  e, -e,  m,  m,  m,  m,  m,  0,  m,  m, -e,  e,  0,  0,  0 },
        {  e, -e,  m,  m,  m,  m,  m,  m,  m, -e,  e,  0,  0,  0,  0 },
        {  e, -e,  m,  m,  m,  m,  m,  m, -e,  e,  0,  0,  0,  0,  0 },
        {  e, -c, -e, -e, -e, -e, -e, -c,  e,  0,  0,  0,  0,  0,  0 },
        {  c,  e,  e,  e,  e,  e,  e,  c,  0,  0,  0,  0,  0,  0,  0 }
    };
    for (int i = 0; i < ARRAY_BOARD_SIZE; i++) {
        for (int j = 0; j < ARRAY_BOARD_SIZE; j++) {
            if (pos->board[i][j] == color) { stability += stabilityWeights[i][j]; }
			else if (pos->board[i][j] == opponent) { stability -= stabilityWeights[i][j]; }
        }
    } score += stability * stabilityweight;

    return score;
}

int minimax(Position *pos, int depth, int alpha, int beta, char maximizingPlayer, char originalPlayer) {
    if (depth == 0 || !canMove(pos, WHITE) && !canMove(pos, BLACK)) {
        return evaluatePosition(pos, originalPlayer,10,5,3);
    }

    if (maximizingPlayer == originalPlayer) {
        int maxEval = -100000;
        for (int i = 0; i < ARRAY_BOARD_SIZE; i++) {
            for (int j = 0; j < ARRAY_BOARD_SIZE; j++) {
                if (isLegal(pos, i, j, maximizingPlayer)) {
                    Position newPos = *pos; Move move = {{i, j}, maximizingPlayer};
                    doMove(&newPos, &move);
                    int eval = minimax(&newPos, depth - 1, alpha, beta, getOtherSide(maximizingPlayer), originalPlayer);
                    maxEval = (eval > maxEval) ? eval : maxEval;
                    alpha = (alpha > eval) ? alpha : eval;
                    if (beta <= alpha && prune) {
                        break;
                    }
                }
            }
        }
        return maxEval;
    } else {
        int minEval = 100000;
        for (int i = 0; i < ARRAY_BOARD_SIZE; i++) {
            for (int j = 0; j < ARRAY_BOARD_SIZE; j++) {
                if (isLegal(pos, i, j, maximizingPlayer)) {
                    Position newPos = *pos; Move move = {{i, j}, maximizingPlayer};
                    doMove(&newPos, &move);
                    int eval = minimax(&newPos, depth - 1, alpha, beta, getOtherSide(maximizingPlayer), originalPlayer);
                    minEval = (eval < minEval) ? eval : minEval;
                    beta = (beta < eval) ? beta : eval;
                    if (beta <= alpha && prune) {
                        break;
                    }
                }
            }
        }
        return minEval;
    }
}

void playMinmax( void ) {
    int bestValue = -100000;
    Move bestMove = {{NULL_MOVE, NULL_MOVE}, myColor};

    for (int i = 0; i < ARRAY_BOARD_SIZE; i++) {
        for (int j = 0; j < ARRAY_BOARD_SIZE; j++) {
            if (isLegal(&gamePosition, i, j, myColor)) {
                Position newPos = gamePosition; Move move = {{i, j}, myColor};
                doMove(&newPos, &move);
                int moveValue = minimax(&newPos, 3, -100000, 100000, getOtherSide(myColor), myColor);
                if (moveValue >= bestValue) {
                    bestValue = moveValue;
                    bestMove = move;
                }
            }
        }
    }

    myMove = bestMove;
}

int main( int argc, char ** argv )
{
	int c;
	void (*playMove)();
	opterr = 0;
	playMove = playMinmax;

	while( ( c = getopt ( argc, argv, "i:p:n:rth" ) ) != -1 )
		switch( c )
		{
			case 'h':
				printf( "[-i ip] [-p port] [-n name] [-r] [-t]\n/ Name max %d chars\n/ -r activates random instead of minimax\n/ -t deactivates pruning\n", MAX_NAME_LENGTH );
				return 0;
			case 'i':
				ip = optarg;
				break;
			case 'p':
				port = optarg;
				break;
			case 'n':
				agentName = optarg;
				break;
			case 'r':
				playMove = playRandom;
				break;
			case 't':
				prune = 1;
				break;
			case '?':
				if( optopt == 'i' || optopt == 'p' || optopt == 'n' )
					printf( "Option -%c requires an argument.\n", ( char ) optopt );
				else if( isprint( optopt ) )
					printf( "Unknown option -%c\n", ( char ) optopt );
				else
					printf( "Unknown option character -%c\n", ( char ) optopt );
				return 1;
			default:
			return 1;
		}

	connectToTarget( port, ip, &mySocket );

/**********************************************************/
// used in random
	srand( time( NULL ) );
/**********************************************************/

	while( 1 )
	{

		msg = recvMsg( mySocket );

		switch ( msg )
		{
			case NM_REQUEST_NAME:		//server asks for our name
				sendName( agentName, mySocket );
				break;

			case NM_NEW_POSITION:		//server is trying to send us a new position
				getPosition( &gamePosition, mySocket );
				printPosition( &gamePosition );
				break;

			case NM_COLOR_W:			//server informs us that we have WHITE color
				myColor = WHITE;
				break;

			case NM_COLOR_B:			//server informs us that we have BLACK color
				myColor = BLACK;
				break;

			case NM_PREPARE_TO_RECEIVE_MOVE:	//server informs us that he will now send us opponent's move
				getMove( &moveReceived, mySocket );
				moveReceived.color = getOtherSide( myColor );
				doMove( &gamePosition, &moveReceived );		//play opponent's move on our position
				printPosition( &gamePosition );
				break;

			case NM_REQUEST_MOVE:		//server requests our move
				myMove.color = myColor;


				if( !canMove( &gamePosition, myColor ) )
				{
					myMove.tile[ 0 ] = NULL_MOVE;		// we have no move ..so send null move
				}
				else
				{
					playMove();
				}

				sendMove( &myMove, mySocket );			//send our move
				doMove( &gamePosition, &myMove );		//play our move on our position
				printPosition( &gamePosition );
				break;

			case NM_QUIT:			//server wants us to quit...we shall obey
				close( mySocket );
				return 0;
		}

	} 

	return 0;
}

/*
        B B B B B B B B 
       B B B B B B B B B 
      W B W W W W B B B B 
     W W B B W W W B B B B 
    W B W B B W W W B B B B 
   W B W B B B W B W W B W B 
  W B W B B B B B B W W W W B 
 W W W B W W W W W W W W W W B 
  W W B B W W W B W B W W B B 
   W B B W W W B B B B B B B 
    W B B W W B B B W W B B 
     W B W W W W W W W B B 
      W B W W W B W W B B 
       W B W W W W W B B 
        W B B B B B B B

        B B B B B B B B 
       B B B B B B B B B 
      W B W W W W B B B B 
     W W B B W W W B B B B 
    W B W B B W W W B B B B 
   W B W B B B W B W W B W B 
  W B W B B B B B B W W W W B 
 W W W B W W W W W W W W W W B 
  W W B B W W W B W B W W B B 
   W B B W W W B B B B B B B 
    W B B W W B B B W W B B 
     W B W W W W W W W B B 
      W B W W W B W W B B 
       W B W W W W W B B 
        W B B B B B B B		
*/




