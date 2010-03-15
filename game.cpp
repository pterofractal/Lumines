//---------------------------------------------------------------------------
//
// CS488 -- Introduction to Computer Graphics
//
// game.hpp/game.cpp
//
// An engine that implements a falling blocks game.  You won't need to 
// modify these files unless you decide to enhance the underlying game
// logic.
//
// University of Waterloo Computer Graphics Lab / 2003
//
//---------------------------------------------------------------------------

#include <algorithm>

#include "game.hpp"
int counter = 0;
static const Piece PIECES[] = {
  Piece(
        "...."
        ".xx."
        ".xx."
        "....", 0,			1,1,1,1), // Blue
  Piece(
        "...."
        ".xx."
        ".xo."
        "....", 1,			1,1,1,1), // purple 
  Piece(
        "...."
        ".xx."
        ".oo."
        "....", 2,			1,1,1,1), // orange
  Piece(
        "...."
        ".ox."
        ".oo."
        "....", 3,			1,1,1,1), // green
  Piece(
        "...."
        ".oo."
        ".oo."
        "....", 4,			1,1,1,1), // red
  Piece(
        "...."
        ".xo."
        ".ox."
        "....", 5,			1,1,1,1), // pink
/*
  Piece(
        "...."
        ".xx."
        ".xx."
        "....", 6,			1,1,1,1) // yellow*/
};

Piece::Piece(const char *desc, int cindex, 
              int left, int top, int right, int bottom)
{
  std::copy(desc, desc + 16, desc_);
  cindex_ = cindex;
  margins_[0] = left;
  margins_[1] = top;
  margins_[2] = right;
  margins_[3] = bottom;
}

Piece::Piece()
{}

Piece& Piece::operator =(const Piece& other)
{
  std::copy(other.desc_, other.desc_ + 16, desc_);
  std::copy(other.margins_, other.margins_ + 4, margins_);
  cindex_ = other.cindex_;
  return *this;
}

int Piece::getLeftMargin() const
{
  return margins_[0];
}

int Piece::getTopMargin() const
{
  return margins_[1];
}

int Piece::getRightMargin() const
{
  return margins_[2];
}

int Piece::getBottomMargin() const
{
  return margins_[3];
}

int Piece::getColourIndex(int row, int col) const
{
	if (desc_[ row*4 + col ] == 'x')
		return 1;
	else if (desc_[ row*4 + col ] == 'o')
		return 2;
	
	return 0;
}

Piece Piece::rotateCW() const
{
  char ndesc[16];
  getColumnRev(0, (char*)ndesc);
  getColumnRev(1, (char*)(ndesc+4));
  getColumnRev(2, (char*)(ndesc+8));
  getColumnRev(3, (char*)(ndesc+12));

  return Piece(ndesc, cindex_,
		margins_[3], margins_[0], margins_[1], margins_[2]);
}

Piece Piece::rotateCCW() const
{
  char ndesc[16];
  getColumn(3, (char*)ndesc);
  getColumn(2, (char*)(ndesc+4));
  getColumn(1, (char*)(ndesc+8));
  getColumn(0, (char*)(ndesc+12));

  return Piece(ndesc, cindex_,
		margins_[1], margins_[2], margins_[3], margins_[0]);
}

bool Piece::isOn(int row, int col) const
{
  return desc_[ row*4 + col ] == 'x' || desc_[ row*4 + col ] == 'o';
}

void Piece::getColumn(int col, char *buf) const
{
  buf[0] = desc_[col];
  buf[1] = desc_[col+4];
  buf[2] = desc_[col+8];
  buf[3] = desc_[col+12];
}

void Piece::getColumnRev(int col, char *buf) const
{
  buf[0] = desc_[col+12];
  buf[1] = desc_[col+8];
  buf[2] = desc_[col+4];
  buf[3] = desc_[col];
}

Game::Game(int width, int height)
  : board_width_(width)
	, board_height_(height)
	, stopped_(false)
	, linesCleared_(0)
	, score_(0)
{
  int sz = board_width_ * (board_height_+4);

  board_ = new int[ sz ];
  std::fill(board_, board_ + sz, -1);
  generateNewPiece();
}

void Game::reset()
{
	stopped_ = false;
	std::fill(board_, board_ + (board_width_*(board_height_+4)), -1);
	linesCleared_ = 0;
	score_ = 0;
	generateNewPiece();
}

Game::~Game()
{
  delete [] board_;
}

int Game::get(int r, int c) const
{
  return board_[ r*board_width_ + c ];
}

int& Game::get(int r, int c) 
{
  return board_[ r*board_width_ + c ];
}

bool Game::doesPieceFit(const Piece& p, int x, int y) const
{
  if(x + p.getLeftMargin() < 0) {
    return false;
  }

  if(x + 3 - p.getRightMargin() >= board_width_) {
    return false;
  }

  if(y + p.getBottomMargin() < 3) {
    return false;
  }

  for(int r = 0; r < 4; ++r) {
    for(int c = 0; c < 4; ++c) {
      if(p.isOn(r, c)) {
        if(get(y-r, x+c) != -1) {
          return false;
        }
      }
    }
  }

  return true;
}

void Game::removePiece(const Piece& p, int x, int y) 
{
  for(int r = 0; r < 4; ++r) {
    for(int c = 0; c < 4; ++c) {
      if(p.isOn(r, c)) {
        get(y-r, x+c) = -1;
      }
    }
  }
}

void Game::removeRow(int y)
{
  for(int r = y + 1; r < board_height_ + 4; ++r) {
    for(int c = 0; c < board_width_; ++c) {
      get(r-1, c) = get(r, c);
    }
  }

  for(int c = 0; c < board_width_; ++c) {
    get(board_height_+3, c) = -1;
  }
}

void Game::markBlocksForClearing() 
{
  // This method is implemented in a brain-dead way.  Repeatedly
  // walk up from the bottom of the well, removing the first full 
  // row, stopping when there are no more full rows.  It could be
  // made much faster.  Sue me.
	for (int r = 0; r<board_height_ + 3; ++r)
	{
		for (int c = 0; c<board_width_ - 1; ++c)
		{
			for (int k = 1;k<3;k++)
			{
				int	l = k + 2;
				if ((get(r, c) == k || get(r, c) == l) && 
					(get(r+1, c) == k || get(r+1, c) == l) && 
					(get(r, c+1) == k || get(r, c+1) == l) && 
					(get(r+1, c+1) == k || get(r+1, c+1) == l)) 
				{
					get(r, c) = l;
					get(r+1, c) = l;
					get(r, c+1) = l;
					get(r+1, c+1) = l;
				}
			}
		}
	}
}

int Game::collapse()
{
	//removePiece(piece_, px_, py_);
	for (int r = board_height_ + 2; r>= 0; --r)
	{
	//	for (int c = 0; c<clearBarPos - 1; ++c)
	//	{	
				int c = (int)clearBarPos;
				
				if (get(r, c) == 3 || get(r, c) == 4 )
				{
					// Collapse
					get(r, c) = -1;
					pullDown(r, c);
				}
		//}
	}
	//placePiece(piece_, px_, py_);
	// Pull pieces down
	return 0;
}

void Game::pullDown(int y, int x)
{
	for(int r = y + 1; r < board_height_ + 4; ++r) 
	{
		get(r-1, x) = get(r, x);
	}
}
void Game::placePiece(const Piece& p, int x, int y)
{
  for(int r = 0; r < 4; ++r) {
    for(int c = 0; c < 4; ++c) {
      if(p.isOn(r, c)) {
        get(y-r, x+c) = p.getColourIndex(r, c);
      }
    }
  }
//dropShadowPiece();
}
	
void Game::generateNewPiece() 
{
  piece_ = PIECES[ rand() % 6 ];

  int xleft = (board_width_-3) / 2;

  px_ = xleft;
  py_ = board_height_ + 3 - piece_.getBottomMargin();

	shadowPiece_ = piece_;
	sx_ = px_;
	sy_ = py_;
	
  placePiece(piece_, px_, py_);
}

int Game::tick()
{
	if(stopped_) 
	{
		return -1;
	}
	
	removePiece(piece_, px_, py_);
	collapse();
	moveClearBar();
	if (counter < 10)
	{
		counter++;
		placePiece(piece_, px_, py_);
		return 0;
	}		
	
	counter = 0;	
	int ny = py_ - 1;
		
	if(!doesPieceFit(piece_, px_, ny)) 
	{
		// Must finish off with this piece
		markBlocksForClearing();
		placePiece(piece_, px_, py_);
		if(py_ >= board_height_ + 1) 
		{
	    	// you lose.
	    	stopped_ = true;
	    	return -1;
		} 
		else
		{
			// break piece and keep moving down if need be

			// The right side can drop more
				if(get(ny-2, px_+1) != -1 && get(ny-2, px_+2) == -1)  
			{						
			/*	Piece temp = piece_;
				temp.removeHalf(1);
				removePiece(piece_, px_, py_);
			  	placePiece(temp, px_, py_);
				while(true) 
				{
					if(get(ny-2, px_+2) != -1) 
					{
			      		break;
			    	}
					--ny;
			  	}
				piece_.removeHalf(0);
				++ny;
			  	placePiece(piece_, px_, ny);*/						
				dropPiece(0);
			}
			else if(get(ny-2, px_+1) == -1 && get(ny-2, px_+2) != -1)  
			{
				dropPiece(1);
			}
	    	int rm = 0;
/*			int level = 1 + linesCleared_ / 10;
			switch (rm)
			{
				case 0:
					score_ += 10 * level;
					break;
				case 1:
					score_ += rm * 100 * level;
					break;
				case 2:
				 	score_ += rm * 300 * level;
					break;
				case 3:
				 	score_ += rm * 500 * level;
					break;
				case 4:
				 	score_ += rm * 800 * level;
					break;
			}
			linesCleared_ += rm;*/
	    	generateNewPiece();
	    	return rm;
		}
	}
	else 
	{
		markBlocksForClearing();
		placePiece(piece_, px_, ny);
		py_ = ny;
		return 0;
	}
}

void Game::dropPiece(int side)
{
	int ny = py_ - 1;
	Piece temp = piece_;
	temp.removeHalf((side + 1)%2);
	removePiece(piece_, px_, py_);
  	placePiece(temp, px_, py_);
	while(true) 
	{
		if(get(ny-2, px_ + 1 + (side + 1)%2) != -1) 
		{
      		break;
    	}
		--ny;
  	}
	piece_.removeHalf(side);
	++ny;
  	placePiece(piece_, px_, ny);
}
bool Game::moveLeft()
{
  // Most of the piece movement methods work like this:
  //  1. remove the piece from the board.
  // 	2. does the piece fit in its new configuration?
  //	3a. if yes, add it to the board in its new configuration.
  //	3b. if no, put it back where it was.
  // Simple and sort of silly, but satisfactory.

  int nx = px_ - 1;

  removePiece(piece_, px_, py_);
  if(doesPieceFit(piece_, nx, py_)) {
    placePiece(piece_, nx, py_);
    px_ = nx;
    return true;
  } else {
    placePiece(piece_, px_, py_);
    return false;
  }
}

bool Game::moveRight()
{
  int nx = px_ + 1;

  removePiece(piece_, px_, py_);
  if(doesPieceFit(piece_, nx, py_)) {
    placePiece(piece_, nx, py_);
    px_ = nx;
    return true;
  } else {
    placePiece(piece_, px_, py_);
    return false;
  }
}

bool Game::drop()
{
  removePiece(piece_, px_, py_);
  int ny = py_;

	while(true) 
	{
    	--ny;
		score_ += 1 + (linesCleared_ / 10);
    	if(!doesPieceFit(piece_, px_, ny)) 
		{
      		break;
    	}
  	}

	++ny;
  	placePiece(piece_, px_, ny);

	if(ny == py_) 
	{
    	return false;
  	}
	else 
	{
    	py_ = ny;
		return true;
  	}
}

bool Game::rotateCW() 
{
	removePiece(piece_, px_, py_);
	//removePiece(shadowPiece_, sx_, sy_);
	Piece npiece = piece_.rotateCW();

	if(doesPieceFit(npiece, px_, py_)) 
	{
		shadowPiece_ = npiece;
		placePiece(npiece, px_, py_);
		piece_ = npiece;
		return true;
	} 
	else 
	{
		placePiece(piece_, px_, py_);
		return false;
	}
}

bool Game::rotateCCW() 
{
	removePiece(piece_, px_, py_);
//	removePiece(shadowPiece_, sx_, sy_);
	Piece npiece = piece_.rotateCCW();
	if(doesPieceFit(npiece, px_, py_)) 
	{
		shadowPiece_ = npiece;
		placePiece(npiece, px_, py_);
		piece_ = npiece;
		return true;
	} 
	else 
	{
		placePiece(piece_, px_, py_);
		return false;
	}
}

void Game::dropShadowPiece()
{
	removePiece(shadowPiece_, sx_, sy_);
	int ny = sy_;
	sx_ = px_;
	while(true) 
	{
		--ny;
		if(!doesPieceFit(shadowPiece_, sx_, ny))
		  break;
	}

	++ny;

	for(int r = 0; r < 4; ++r) 
	{
		for(int c = 0; c < 4; ++c) 
		{
			if(shadowPiece_.isOn(r, c))
				get(ny-r, sx_+c) = shadowPiece_.getColourIndex(r, c);
		}
	}

	if(ny != sy_)
	  sy_ = ny;
}

void Game::moveClearBar()
{
	if (clearBarPos > board_width_)
	{
		clearBarPos = 0;
	}
	clearBarPos += 0.2;
//	collapse();
}

