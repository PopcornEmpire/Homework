#include "cellbutton.h"

#include <QAction>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QGridLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QSettings>
#include <QTextStream>
#include <QToolBar>

enum Language
{
	Russian,
	English
};

class MainWindow : public QMainWindow
{
	Q_OBJECT

  public:
	MainWindow(int argc, char *argv[], QWidget *parent = nullptr);
	~MainWindow();

  protected:
	void closeEvent(QCloseEvent *event) override;

  private slots:
	void handleLeftClick(int row, int col);
	void handleRightClick(int row, int col);
	void handleMiddleClick(int row, int col);
	void startNewGame();
	void startNewGameWithSettings();
	void reverseMode();
	void increaseButtonSize();
	void decreaseButtonSize();
	void changeLanguage();

  private:
	void setupMenu();
	void setupToolBar();
	void generateMines(int excludedRow, int excludedCol);
	int countAdjacentMines(int row, int col);
	void revealCell(int row, int col);
	void revealAllMines(int clickedRow, int clickedCol);
	void checkForWin();
	void saveGameState();
	void loadGameState();
	void updateMineCounter();
	void recreateField();
	void updateButtonHandlers();
	void createDebugWindow();
	void updateInterfaceTexts();
	void showHelpMessage();

	QGridLayout *gridLayout;
	QLabel *mineCounterLabel;
	QSettings *settings;
	QAction *newGameAction;
	QAction *newGameWithSettingsAction;
	QAction *leftHandedAction;
	QAction *increaseButtonSizeAction;
	QAction *decreaseButtonSizeAction;
	QAction *changeLanguageAction;
	QAction *helpAction;
	QMenu *gameMenu;

	QAction *newGameToolAction;
	QAction *newGameSettingsToolAction;
	QAction *leftHandedToolAction;
	QAction *increaseButtonSizeToolAction;
	QAction *decreaseButtonSizeToolAction;
	QAction *changeLanguageToolAction;
	QAction *helpToolAction;

	int flagsPlaced;
	bool gameOver;
	int fieldWidth;
	int fieldHeight;
	int mineCount;
	bool minesGenerated;
	bool leftHanded;
	bool dbgMode;
	int lastClickedMineRow;
	int lastClickedMineCol;
	int buttonSize;

	Language currentLanguage;

	QVector< QVector< CellButton * > > buttons;
	QVector< QVector< int > > mines;
	QVector< QVector< int > > cellStates;
};
