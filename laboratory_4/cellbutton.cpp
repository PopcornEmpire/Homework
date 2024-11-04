#include "cellbutton.h"

void CellButton::setPosition(int row, int col)
{
	this->row = row;
	this->col = col;
}
void CellButton::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		emit leftClicked(row, col);
	}
	else if (event->button() == Qt::RightButton)
	{
		emit rightClicked(row, col);
	}
	else if (event->button() == Qt::MiddleButton)
	{
		emit middleClicked(row, col);
	}
}
