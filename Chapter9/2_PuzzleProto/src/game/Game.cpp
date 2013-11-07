#include "Game.h"

void Puzzle::Retoss( int W, int H )
{
	FColumns = W;
	FRows    = H;
	FTiles.resize( FColumns * FRows );

	// init tiles
	for ( int i = 0; i != FColumns; i++ )
		for ( int j = 0; j != FRows; j++ )
		{
			FTiles[j * FColumns + i] = clTile( i, FRows - j - 1, FColumns, FRows );
		}

	// toss the image
	int TotalTiles = FColumns * FRows;
	for( int idx1 = TotalTiles - 1 ; idx1 > 0 ; idx1--)
	{
		int idx2 = Linderdaum::Math::RandomInRange( 0, idx1 );

		int Tile1i = idx1 % FColumns;
		int Tile1j = idx1 / FColumns;
		int Tile2i = idx2 % FColumns;
		int Tile2j = idx2 / FColumns;

		SwapTiles( Tile1i, Tile1j, Tile2i, Tile2j );
	}
/*
	// toss image
	for ( int i = 0; i != FColumns; i++ )
		for ( int j = 0; j != FRows; j++ )
		{
			int NewI = Linderdaum::Math::RandomInRange( i, FColumns - 1 );
			int NewJ = Linderdaum::Math::RandomInRange( j, FRows - 1    );
			SwapTiles( i, j, NewI, NewJ );
		}
*/
	FClickedI = FClickedJ = -1;
}

void Puzzle::Timer( float DeltaSeconds )
{
	for ( int i = 0; i != FColumns; i++ )
		for ( int j = 0; j != FRows; j++ )
		{
			GetTile( i, j )->Update( DeltaSeconds );
		}
}

bool Puzzle::IsComplete() const
{
	for ( int i = 0; i != FColumns; i++ )
		for ( int j = 0; j != FRows; j++ )
		{
			clTile* T = GetTile( i, j );

			if ( T->FOriginX != i || T->FOriginY != j )
			{
				return false;
			}
		}

	return true;
}

void Puzzle::OnKey( float mx, float my, bool KeyState )
{
	int i = ( int )floor( mx * FColumns );
	int j = ( int )floor( my * FRows );
	int MouseI = ( i >= 0 && i < FColumns ) ? i : -1;
	int MouseJ = ( j >= 0 && j < FRows ) ? j : -1;

	FMovingImage = KeyState;

	if ( FMovingImage )
	{
		FClickedI = MouseI;
		FClickedJ = MouseJ;

		if ( FClickedI >= 0 && FClickedJ >= 0 && FClickedI < FColumns && FClickedJ < FRows )
		{
			FOfsX = ( ( float )FClickedI / FColumns - mx );
			FOfsY = ( ( float )FClickedJ / FRows    - my );
		}
		else
		{
			FClickedI = FClickedJ = -1;
		}
	}
	else
	{
		bool NewPosition   = ( MouseI != FClickedI || MouseJ != FClickedJ );
		bool ValidPosition1 = ( FClickedI >= 0 && FClickedJ >= 0 && FClickedI < FColumns && FClickedJ < FRows );
		bool ValidPosition2 = ( MouseI >= 0 && MouseJ >= 0 && MouseI < FColumns && MouseJ < FRows );

		if ( NewPosition && ValidPosition1 && ValidPosition2 )
		{
			int dX = MouseI - FClickedI, dY = MouseJ - FClickedJ;
			SwapTiles( FClickedI, FClickedJ, MouseI, MouseJ );
		}

		if ( IsComplete() ) { /* invoke event handler */ }

		FClickedI = FClickedJ = -1;
	}
}
