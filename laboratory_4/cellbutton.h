#ifndef CELLBUTTON_H
#define CELLBUTTON_H

#include <QMouseEvent>
#include <QPushButton>

class CellButton : public QPushButton
{
	Q_OBJECT

  signals:
	void leftClicked(int row, int col);
	void rightClicked(int row, int col);
	void middleClicked(int row, int col);

  public:
	void setPosition(int row, int col);

  private:
	int row;
	int col;

  protected:
	void mousePressEvent(QMouseEvent *event) override;
};

#endif
