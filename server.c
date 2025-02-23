#include "global.h"
#include "board.h"
#include "move.h"
#include "comm.h"
#include "gameServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/**********************************************************/
int main( int argc, char **argv )
{



	int c;
	opterr = 0;

	while( ( c = getopt( argc, argv, "p:g:hs" ) ) != -1 )
		switch( c )
		{
			case 'h':
				printf( "[-p port] [-g number_of_games] [-s (swap color after each game)]\n" );
				return 0;
			case 'p':
				port = optarg;
				break;
			case 'g':
				numberOfGames = atoi( optarg );
				break;
			case 's':
				swapAfterEachGame = TRUE;
				break;
			case '?':
				if( optopt == 'p' || optopt == 'g' )
					printf( "Option -%c requires an argument.\n", ( char ) optopt );
				else if( isprint( optopt ) )
					printf( "Unknown option -%c\n", ( char ) optopt );
				else
					printf( "Unknown option character -%c\n", ( char ) optopt );
				return 1;
			default:
			return 1;
		}



	listenToSocket( port, &serverSocket );

	playerOne.playerSocket = acceptConnection( serverSocket );
	playerTwo.playerSocket = acceptConnection( serverSocket );

	//first who connects gets white color
	playerOne.color = WHITE;
	playerTwo.color = BLACK;

	sendMsg( NM_COLOR_W, playerOne.playerSocket );
	sendMsg( NM_COLOR_B, playerTwo.playerSocket );


	//request names
	sendMsg( NM_REQUEST_NAME, playerOne.playerSocket );
	getName( playerOne.name, playerOne.playerSocket );

	sendMsg( NM_REQUEST_NAME, playerTwo.playerSocket );
	getName( playerTwo.name, playerTwo.playerSocket );


	int i;

	for( i = 0; i < numberOfGames; i++ )
	{

		initPosition( &gamePosition );
		printPosition( &gamePosition );

		//sending position
		sendMsg( NM_NEW_POSITION, playerOne.playerSocket );
		sendPosition( &gamePosition, playerOne.playerSocket );

		sendMsg( NM_NEW_POSITION, playerTwo.playerSocket );
		sendPosition( &gamePosition, playerTwo.playerSocket );

		while( 1 )		//inside a game
		{

			if( playerOne.color == gamePosition.turn )
			{
				playingPlayer = &playerOne;
				waitingPlayer = &playerTwo;
			}
			else
			{
				playingPlayer = &playerTwo;
				waitingPlayer = &playerOne;
			}

			//get move
			sendMsg( NM_REQUEST_MOVE, playingPlayer->playerSocket );
			getMove( &tempMove, playingPlayer->playerSocket );

			tempMove.color = playingPlayer->color;

			//check legality
			if( !canMove( &gamePosition, playingPlayer->color ) )	//if that player cannot move, the only legal move is null
			{
				if( tempMove.tile[ 0 ] != NULL_MOVE )	//technical loss
				{
					printf( "Player: %s tried an illegal move and lost the game!\nIllegal move:", playingPlayer->name );
					printf( "( %d, %d )", tempMove.tile[ 0 ], tempMove.tile[ 1 ] );
					printf("\n");
					break;
				}
			}
			else
			{
				if( !isLegalMove( &gamePosition, &tempMove ) )
				{
					//technical loss
					printf( "Player: %s tried an illegal move and lost the game!\nIllegal move:", playingPlayer->name );

					if( tempMove.tile[ 0 ] == NULL_MOVE )	//since we can move, null move is illegal
						printf( "NULL MOVE" );
					else
						printf( "( %d, %d )", tempMove.tile[ 0 ], tempMove.tile[ 1 ] );
					printf("\n");
					break;
				}
			}

			//we have a legal move
			doMove( &gamePosition, &tempMove );
			printPosition( &gamePosition );

			//check victory conditions
			if( !canMove( &gamePosition, WHITE ) && !canMove( &gamePosition, BLACK ) )	//if none can move..game ended
			{
				printf( "Game ended!\n" );

				if( gamePosition.score[ WHITE ] - gamePosition.score[ BLACK ] > 0 )
				{
					//white won
					if( playerOne.color == WHITE )
					{
						printf( "WHITE WON! (%s) Score W:%d B:%d\n", playerOne.name,gamePosition.score[ WHITE ], gamePosition.score[ BLACK ] );
					}
					else
					{
						printf( "WHITE WON! (%s) Score W:%d B:%d\n", playerTwo.name, gamePosition.score[ WHITE ], gamePosition.score[ BLACK ] );
					}
				}
				else if( gamePosition.score[ WHITE ] - gamePosition.score[ BLACK ] < 0 )
				{
					//Black won
					if( playerOne.color == BLACK )
					{
						printf( "BLACK WON! (%s) Score W:%d B:%d\n", playerOne.name, gamePosition.score[ WHITE ], gamePosition.score[ BLACK ] );
					}
					else
					{
						printf( "BLACK WON! (%s) Score W:%d B:%d\n", playerTwo.name, gamePosition.score[ WHITE ], gamePosition.score[ BLACK ] );
					}
				}
				else
					printf( "DRAW! Score W:%d B:%d\n", gamePosition.score[ WHITE ], gamePosition.score[ BLACK ] );

				break;
			}

			//send move to the other player
			sendMsg( NM_PREPARE_TO_RECEIVE_MOVE, waitingPlayer->playerSocket );
			sendMove( &tempMove, waitingPlayer->playerSocket );


		}

		if( swapAfterEachGame == TRUE )		//swap colors if flag is TRUE
		{
			if( playerOne.color == BLACK )
			{
				sendMsg( NM_COLOR_W, playerOne.playerSocket );
				sendMsg( NM_COLOR_B, playerTwo.playerSocket );
				playerOne.color = WHITE;
				playerTwo.color = BLACK;
			}
			else
			{
				sendMsg( NM_COLOR_B, playerOne.playerSocket );
				sendMsg( NM_COLOR_W, playerTwo.playerSocket );
				playerOne.color = BLACK;
				playerTwo.color = WHITE;
			}
		}


	}

	sendMsg( NM_QUIT, playerOne.playerSocket );
	sendMsg( NM_QUIT, playerTwo.playerSocket );

	return 0;
}






