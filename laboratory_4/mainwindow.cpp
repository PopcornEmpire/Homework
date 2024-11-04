#include "mainwindow.h"

// в режиме левша после загрузки из сохраненного состояния
// снимается синий покрас (если режим левши был активирован),
// но функционал остается в лююом случае. Верить надо надписям, которые высвечиваются - они верные

// поля при растягивание особо не меняются, но есть 2 кнопки : больше , меньше, которые изменяют
// размер всех кнопок, кроме вылизающего меню при нажатие на кнопку игра

// еще могу добавить, что размер поля ограничен от 5*5 до 20*20 , но это легко изменить если что
// все надписи касающиеся dbg мода на русском

// смена языка реализована простыми if , без встроенных функций перевода Qt

MainWindow::MainWindow(int argc, char *argv[], QWidget *parent) :
	QMainWindow(parent), flagsPlaced(0), gameOver(false), fieldWidth(10), fieldHeight(10), mineCount(10),
	minesGenerated(false), leftHanded(false), dbgMode(false), lastClickedMineRow(-1), lastClickedMineCol(-1), buttonSize(40)
{
	QFont font;
	font.setPointSize(9);
	this->setFont(font);

	if (argc > 1 && QString(argv[1]) == "dbg")
	{
		dbgMode = true;
		QMessageBox::information(this, "Режим отладки", "Вы запустили режим отладки");
	}

	settings = new QSettings("Minesweeper", "MinesweeperSettings", this);

	QWidget *centralWidget = new QWidget(this);
	setCentralWidget(centralWidget);

	gridLayout = new QGridLayout(centralWidget);

	mineCounterLabel = new QLabel("Количество оставшихся мин: " + QString::number(mineCount), this);
	mineCounterLabel->setAlignment(Qt::AlignLeft);

	QFontMetrics fontMetrics(mineCounterLabel->font());
	mineCounterLabel->setFixedWidth(fontMetrics.horizontalAdvance(mineCounterLabel->text()) + 20);

	gridLayout->addWidget(mineCounterLabel, 0, 0, 1, fieldWidth, Qt::AlignLeft);
	setupToolBar();
	setupMenu();

	loadGameState();
	updateMineCounter();

	setWindowTitle("Сапёр");
}

MainWindow::~MainWindow()
{
	delete settings;
}

void MainWindow::setupMenu()
{
	gameMenu = menuBar()->addMenu("Игра");

	newGameAction = new QAction("Новая игра", this);
	connect(newGameAction, &QAction::triggered, this, &MainWindow::startNewGame);
	gameMenu->addAction(newGameAction);

	newGameWithSettingsAction = new QAction("Новая игра с настройками", this);
	connect(newGameWithSettingsAction, &QAction::triggered, this, &MainWindow::startNewGameWithSettings);
	gameMenu->addAction(newGameWithSettingsAction);

	leftHandedAction = new QAction("Левша", this);
	leftHandedAction->setCheckable(true);
	leftHandedAction->setChecked(leftHanded);
	connect(leftHandedAction, &QAction::triggered, this, &MainWindow::reverseMode);
	gameMenu->addAction(leftHandedAction);

	increaseButtonSizeAction = new QAction("Больше", this);
	connect(increaseButtonSizeAction, &QAction::triggered, this, &MainWindow::increaseButtonSize);
	gameMenu->addAction(increaseButtonSizeAction);

	decreaseButtonSizeAction = new QAction("Меньше", this);
	connect(decreaseButtonSizeAction, &QAction::triggered, this, &MainWindow::decreaseButtonSize);
	gameMenu->addAction(decreaseButtonSizeAction);

	changeLanguageAction = new QAction("Смена языка", this);
	connect(changeLanguageAction, &QAction::triggered, this, &MainWindow::changeLanguage);
	gameMenu->addAction(changeLanguageAction);

	helpAction = new QAction("Помощь", this);
	connect(helpAction, &QAction::triggered, this, &MainWindow::showHelpMessage);
	gameMenu->addAction(helpAction);
}

void MainWindow::changeLanguage()
{
	if (currentLanguage == Russian)
	{
		currentLanguage = English;
	}
	else
	{
		currentLanguage = Russian;
	}
	updateInterfaceTexts();
}

void MainWindow::setupToolBar()
{
	QToolBar *toolBar = addToolBar("Основные действия");
	toolBar->setMovable(false);

	newGameToolAction = new QAction("Новая игра", this);
	connect(newGameToolAction, &QAction::triggered, this, &MainWindow::startNewGame);
	toolBar->addAction(newGameToolAction);

	newGameSettingsToolAction = new QAction("Настройки", this);
	connect(newGameSettingsToolAction, &QAction::triggered, this, &MainWindow::startNewGameWithSettings);
	toolBar->addAction(newGameSettingsToolAction);

	leftHandedToolAction = new QAction("Левша", this);
	leftHandedToolAction->setCheckable(true);
	leftHandedToolAction->setChecked(leftHanded);
	connect(leftHandedToolAction, &QAction::triggered, this, &MainWindow::reverseMode);
	toolBar->addAction(leftHandedToolAction);

	increaseButtonSizeToolAction = new QAction("Больше", this);
	connect(increaseButtonSizeToolAction, &QAction::triggered, this, &MainWindow::increaseButtonSize);
	toolBar->addAction(increaseButtonSizeToolAction);

	decreaseButtonSizeToolAction = new QAction("Меньше", this);
	connect(decreaseButtonSizeToolAction, &QAction::triggered, this, &MainWindow::decreaseButtonSize);
	toolBar->addAction(decreaseButtonSizeToolAction);

	changeLanguageToolAction = new QAction("Смена языка", this);
	connect(changeLanguageToolAction, &QAction::triggered, this, &MainWindow::changeLanguage);
	toolBar->addAction(changeLanguageToolAction);

	helpToolAction = new QAction("Помощь", this);
	connect(helpToolAction, &QAction::triggered, this, &MainWindow::showHelpMessage);
	toolBar->addAction(helpToolAction);
}

void MainWindow::updateInterfaceTexts()
{
	if (currentLanguage == Russian)
	{
		setWindowTitle("Сапёр");
		gameMenu->setTitle("Игра");
		newGameAction->setText("Новая игра");
		newGameWithSettingsAction->setText("Новая игра с настройками");
		leftHandedAction->setText("Левша");
		increaseButtonSizeAction->setText("Больше");
		decreaseButtonSizeAction->setText("Меньше");
		changeLanguageAction->setText("Смена языка");
		helpAction->setText("Помощь");

		newGameToolAction->setText("Новая игра");
		newGameSettingsToolAction->setText("Настройки");
		leftHandedToolAction->setText("Левша");
		increaseButtonSizeToolAction->setText("Больше");
		decreaseButtonSizeToolAction->setText("Меньше");
		changeLanguageToolAction->setText("En");
		helpToolAction->setText("Помощь");
	}
	else
	{
		setWindowTitle("Minesweeper");
		gameMenu->setTitle("Game");
		newGameAction->setText("New Game");
		newGameWithSettingsAction->setText("New Game with Settings");
		leftHandedAction->setText("Left-handed");
		increaseButtonSizeAction->setText("Increase");
		decreaseButtonSizeAction->setText("Decrease");
		changeLanguageAction->setText("Change Language");
		helpAction->setText("Help");

		newGameToolAction->setText("New Game");
		newGameSettingsToolAction->setText("Settings");
		leftHandedToolAction->setText("Left-handed");
		increaseButtonSizeToolAction->setText("Increase");
		decreaseButtonSizeToolAction->setText("Decrease");
		changeLanguageToolAction->setText("Рус");
		helpToolAction->setText("Help");
	}
	updateMineCounter();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	saveGameState();
	event->accept();
}

void MainWindow::generateMines(int excludedRow, int excludedCol)
{
	int placedMines = 0;

	while (placedMines < mineCount)
	{
		int row = QRandomGenerator::global()->bounded(fieldHeight);
		int col = QRandomGenerator::global()->bounded(fieldWidth);

		if (mines[row][col] == 0 && !(row == excludedRow && col == excludedCol))
		{
			mines[row][col] = 1;
			placedMines++;
		}
	}

	minesGenerated = true;

	if (dbgMode)
	{
		createDebugWindow();
	}
}

int MainWindow::countAdjacentMines(int row, int col)
{
	int count = 0;

	for (int i = -1; i <= 1; ++i)
	{
		for (int j = -1; j <= 1; ++j)
		{
			if (i == 0 && j == 0)
				continue;

			int newRow = row + i;
			int newCol = col + j;

			if (newRow >= 0 && newRow < fieldHeight && newCol >= 0 && newCol < fieldWidth)
			{
				if (mines[newRow][newCol] == 1)
				{
					count++;
				}
			}
		}
	}

	return count;
}

void MainWindow::handleLeftClick(int row, int col)
{
	if (gameOver)
		return;

	if (!minesGenerated)
	{
		generateMines(row, col);
	}

	revealCell(row, col);

	checkForWin();
}

void MainWindow::handleRightClick(int row, int col)
{
	// логика обработки вспомогательного клика (название было дано до реализации режима левши)
	QString cellText = buttons[row][col]->text();
	if (cellText == "")
	{
		buttons[row][col]->setText("F");
		flagsPlaced++;
	}
	else if (cellText == "F")
	{
		buttons[row][col]->setText("?");
		flagsPlaced--;
	}
	else if (cellText == "?")
	{
		buttons[row][col]->setText("");
	}

	updateMineCounter();
	checkForWin();
}

void MainWindow::handleMiddleClick(int row, int col)
{
	if (gameOver)
		return;

	if (cellStates[row][col] == 0)
		return;

	int cellNumber = countAdjacentMines(row, col);

	int adjacentFlags = 0;
	for (int i = -1; i <= 1; ++i)
	{
		for (int j = -1; j <= 1; ++j)
		{
			int newRow = row + i;
			int newCol = col + j;
			if (newRow >= 0 && newRow < fieldHeight && newCol >= 0 && newCol < fieldWidth)
			{
				if (buttons[newRow][newCol]->text() == "F")
				{
					adjacentFlags++;
				}
			}
		}
	}

	int newRow;
	int newCol;
	if (adjacentFlags == cellNumber)
	{
		for (int i = -1; i <= 1; ++i)
		{
			for (int j = -1; j <= 1; ++j)
			{
				newRow = row + i;
				newCol = col + j;
				if (newRow >= 0 && newRow < fieldHeight && newCol >= 0 && newCol < fieldWidth)
				{
					if (buttons[newRow][newCol]->text() != "F" && cellStates[newRow][newCol] == 0)
					{
						revealCell(newRow, newCol);
					}
				}
			}
		}
		checkForWin();
	}
}

void MainWindow::reverseMode()
{
	leftHanded = !leftHanded;
	leftHandedAction->setChecked(leftHanded);
	QString status;
	QString title;
	if (currentLanguage == Russian)
	{
		status = leftHanded ? "Активирован режим 'Левша'" : "Деактивирован режим 'Левша'";
		title = "Режим 'Левша'";
	}
	else
	{
		status = leftHanded ? "Left-handed mode activated" : "Left-handed mode deactivated";
		title = "Left-handed Mode";
	}
	QMessageBox::information(this, title, status);
	updateButtonHandlers();
}

void MainWindow::revealCell(int row, int col)
{
	if (gameOver)
		return;

	if (cellStates[row][col] == 1)
		return;

	QString cellText = buttons[row][col]->text();
	if (cellText == "F" || cellText == "?")
		return;

	cellStates[row][col] = 1;

	if (mines[row][col] == 1)
	{
		revealAllMines(row, col);
		if (currentLanguage == Russian)
		{
			QMessageBox::information(this, "Игра окончена", "Вы наступили на мину!");
		}
		else
		{
			QMessageBox::information(this, "The game is over", "You stepped on a mine!");
		}
		gameOver = true;
		return;
	}

	int adjacentMines = countAdjacentMines(row, col);

	if (adjacentMines == 0)
	{
		buttons[row][col]->setEnabled(false);
		buttons[row][col]->setStyleSheet("background-color: lightgray;");

		for (int i = -1; i <= 1; ++i)
		{
			for (int j = -1; j <= 1; ++j)
			{
				int newRow = row + i;
				int newCol = col + j;

				if (i == 0 && j == 0)
					continue;

				if (newRow >= 0 && newRow < fieldHeight && newCol >= 0 && newCol < fieldWidth)
				{
					revealCell(newRow, newCol);
				}
			}
		}
	}
	else
	{
		buttons[row][col]->setText(QString::number(adjacentMines));
		buttons[row][col]->setStyleSheet("background-color: darkgray;");
	}
}

void MainWindow::updateButtonHandlers()
{
	// просто меняю для каждой кнопки сигналы, работает это относительно медленно
	// можно было бы реализовать через метод который просто менял вызов методов
	// правое/левое нажатие в будщем
	for (int row = 0; row < fieldHeight; ++row)
	{
		for (int col = 0; col < fieldWidth; ++col)
		{
			CellButton *button = buttons[row][col];
			disconnect(button, &CellButton::leftClicked, this, nullptr);
			disconnect(button, &CellButton::rightClicked, this, nullptr);
			disconnect(button, &CellButton::middleClicked, this, nullptr);

			if (!leftHanded)
			{
				connect(button, &CellButton::leftClicked, this, [this, row, col]() { handleLeftClick(row, col); });
				connect(button, &CellButton::rightClicked, this, [this, row, col]() { handleRightClick(row, col); });
			}
			else
			{
				connect(button, &CellButton::leftClicked, this, [this, row, col]() { handleRightClick(row, col); });
				connect(button, &CellButton::rightClicked, this, [this, row, col]() { handleLeftClick(row, col); });
			}
			connect(button, &CellButton::middleClicked, this, [this, row, col]() { handleMiddleClick(row, col); });
		}
	}
}

void MainWindow::recreateField()
{
	for (int row = 0; row < buttons.size(); ++row)
	{
		for (int col = 0; col < buttons[row].size(); ++col)
		{
			gridLayout->removeWidget(buttons[row][col]);
			delete buttons[row][col];
		}
	}
	buttons.clear();
	mines.clear();
	cellStates.clear();

	buttons.resize(fieldHeight);
	mines.resize(fieldHeight);
	cellStates.resize(fieldHeight);
	for (int row = 0; row < fieldHeight; ++row)
	{
		buttons[row].resize(fieldWidth);
		mines[row].resize(fieldWidth);
		cellStates[row].resize(fieldWidth);
		for (int col = 0; col < fieldWidth; ++col)
		{
			CellButton *button = new CellButton();
			button->setFixedSize(buttonSize, buttonSize);
			button->setPosition(row, col);
			buttons[row][col] = button;
			gridLayout->addWidget(button, row + 1, col);
			cellStates[row][col] = 0;
		}
	}

	gameOver = false;
	flagsPlaced = 0;
	minesGenerated = false;
	lastClickedMineRow = -1;
	lastClickedMineCol = -1;
	for (int row = 0; row < fieldHeight; ++row)
	{
		for (int col = 0; col < fieldWidth; ++col)
		{
			mines[row][col] = 0;
			buttons[row][col]->setText("");
			buttons[row][col]->setStyleSheet("");
		}
	}

	updateMineCounter();

	updateButtonHandlers();
}

void MainWindow::checkForWin()
{
	if (gameOver)
		return;
	bool won = true;

	for (int row = 0; row < fieldHeight; ++row)
	{
		for (int col = 0; col < fieldWidth; ++col)
		{
			if (mines[row][col] == 0 && cellStates[row][col] == 0)
			{
				won = false;
				break;
			}
		}
		if (!won)
			break;
	}

	if (won)
	{
		if (currentLanguage == Russian)
		{
			QMessageBox::information(this, "Поздравляем!", "Вы выиграли!");
		}
		else
		{
			QMessageBox::information(this, "Congratulations!", "You won!");
		}
		gameOver = true;
		for (int r = 0; r < fieldHeight; ++r)
		{
			for (int c = 0; c < fieldWidth; ++c)
			{
				buttons[r][c]->setEnabled(false);
			}
		}
	}
}

// в начале этот метод открывал только мины, теперь вообще все клетки
void MainWindow::revealAllMines(int clickedRow, int clickedCol)
{
	for (int row = 0; row < fieldHeight; ++row)
	{
		for (int col = 0; col < fieldWidth; ++col)
		{
			if (mines[row][col] == 1)
			{
				buttons[row][col]->setText("M");
				if (row == clickedRow && col == clickedCol)
				{
					buttons[row][col]->setStyleSheet("background-color: red;");
					lastClickedMineRow = row;
					lastClickedMineCol = col;
				}
			}
			else
			{
				int adjacentMines = countAdjacentMines(row, col);
				if (adjacentMines > 0)
				{
					buttons[row][col]->setText(QString::number(adjacentMines));
					buttons[row][col]->setStyleSheet("background-color: darkgray;");
				}
			}
			buttons[row][col]->setEnabled(false);
		}
	}
}

void MainWindow::startNewGame()
{
	recreateField();
}

void MainWindow::startNewGameWithSettings()
{
	int newFieldWidth;
	int newFieldHeight;
	int newMineCount;
	int maxMines;
	bool ok1;
	bool ok2;
	bool ok3;
	if (currentLanguage == Russian)
	{
		newFieldWidth = QInputDialog::getInt(this, "Настройки", "Ширина поля (от 5 до 20):", fieldWidth, 5, 20, 1, &ok1);
		newFieldHeight = QInputDialog::getInt(this, "Настройки", "Высота поля (от 5 до 20):", fieldHeight, 5, 20, 1, &ok2);
	}
	else
	{
		newFieldWidth = QInputDialog::getInt(this, "Settings", "Field width (from 5 to 20):", fieldWidth, 5, 20, 1, &ok1);
		newFieldHeight = QInputDialog::getInt(this, "Settings", "Field height (from 5 to 20):", fieldHeight, 5, 20, 1, &ok2);
	}
	fieldWidth = ok1 ? newFieldWidth : fieldWidth;
	fieldHeight = ok2 ? newFieldHeight : fieldHeight;
	maxMines = fieldWidth * fieldHeight - 1;
	if (currentLanguage == Russian)
	{
		newMineCount =
			QInputDialog::getInt(this, "Настройки", "Количество мин (от 1 до " + QString::number(maxMines) + "):", mineCount, 1, maxMines, 1, &ok3);
	}
	else
	{
		newMineCount =
			QInputDialog::getInt(this, "Settings", "Number of mines (from 1 to " + QString::number(maxMines) + "):", mineCount, 1, maxMines, 1, &ok3);
	}
	mineCount = ok3 ? newMineCount : mineCount;

	recreateField();
}

void MainWindow::increaseButtonSize()
{
	QFont font = this->font();
	font.setPointSize(font.pointSize() + 1);
	this->setFont(font);
	buttonSize += 9;
	for (int row = 0; row < fieldHeight; ++row)
	{
		for (int col = 0; col < fieldWidth; ++col)
		{
			buttons[row][col]->setFixedSize(buttonSize, buttonSize);
		}
	}

	if (dbgMode && minesGenerated)
	{
		createDebugWindow();
	}
}

void MainWindow::decreaseButtonSize()
{
	QFont font = this->font();
	font.setPointSize(font.pointSize() - 1);
	this->setFont(font);
	if (buttonSize > 10)
	{
		buttonSize -= 9;
		for (int row = 0; row < fieldHeight; ++row)
		{
			for (int col = 0; col < fieldWidth; ++col)
			{
				buttons[row][col]->setFixedSize(buttonSize, buttonSize);
			}
		}

		if (dbgMode && minesGenerated)
		{
			createDebugWindow();
		}
	}
}

void MainWindow::createDebugWindow()
{
	QWidget *debugWindow = new QWidget();
	QGridLayout *debugLayout = new QGridLayout(debugWindow);

	for (int row = 0; row < fieldHeight; ++row)
	{
		for (int col = 0; col < fieldWidth; ++col)
		{
			QLabel *label = new QLabel();
			label->setFixedSize(buttonSize, buttonSize);
			label->setAlignment(Qt::AlignCenter);
			if (mines[row][col] == 1)
			{
				label->setText("M");
				label->setStyleSheet("background-color: red;");
			}
			else
			{
				int adjacentMines = countAdjacentMines(row, col);
				if (adjacentMines > 0)
				{
					label->setText(QString::number(adjacentMines));
				}
				label->setStyleSheet("background-color: lightgray;");
			}
			debugLayout->addWidget(label, row, col);
		}
	}

	debugWindow->setLayout(debugLayout);
	debugWindow->setWindowTitle("Отладка: Открытое поле");
	debugWindow->show();
}

void MainWindow::saveGameState()
{
	QString filePath = QCoreApplication::applicationDirPath() + "/game_state.ini";
	QSettings settings(filePath, QSettings::IniFormat);

	settings.beginGroup("General");
	settings.setValue("currentLanguage", static_cast< int >(currentLanguage));
	settings.setValue("fieldWidth", fieldWidth);
	settings.setValue("fieldHeight", fieldHeight);
	settings.setValue("mineCount", mineCount);
	settings.setValue("flagsPlaced", flagsPlaced);
	settings.setValue("gameOver", gameOver);
	settings.setValue("minesGenerated", minesGenerated);
	settings.setValue("leftHanded", leftHanded);
	settings.setValue("lastClickedMineRow", lastClickedMineRow);
	settings.setValue("lastClickedMineCol", lastClickedMineCol);
	settings.setValue("buttonSize", buttonSize);
	settings.endGroup();

	settings.beginWriteArray("Cells");
	int index = 0;
	for (int row = 0; row < fieldHeight; ++row)
	{
		for (int col = 0; col < fieldWidth; ++col)
		{
			settings.setArrayIndex(index);
			settings.setValue("row", row);
			settings.setValue("col", col);
			settings.setValue("mine", mines[row][col]);
			settings.setValue("text", buttons[row][col]->text());
			settings.setValue("enabled", buttons[row][col]->isEnabled());
			settings.setValue("color", buttons[row][col]->styleSheet());
			index++;
		}
	}
	settings.endArray();
}

void MainWindow::loadGameState()
{
	QString filePath = QCoreApplication::applicationDirPath() + "/game_state.ini";
	QSettings settings(filePath, QSettings::IniFormat);

	if (QFile::exists(filePath))
	{
		settings.beginGroup("General");
		currentLanguage = static_cast< Language >(settings.value("currentLanguage", static_cast< int >(Russian)).toInt());
		fieldWidth = settings.value("fieldWidth", fieldWidth).toInt();
		fieldHeight = settings.value("fieldHeight", fieldHeight).toInt();
		mineCount = settings.value("mineCount", mineCount).toInt();
		flagsPlaced = settings.value("flagsPlaced", flagsPlaced).toInt();
		gameOver = settings.value("gameOver", gameOver).toBool();
		minesGenerated = settings.value("minesGenerated", minesGenerated).toBool();
		leftHanded = settings.value("leftHanded", leftHanded).toBool();
		lastClickedMineRow = settings.value("lastClickedMineRow", lastClickedMineRow).toInt();
		lastClickedMineCol = settings.value("lastClickedMineCol", lastClickedMineCol).toInt();
		buttonSize = settings.value("buttonSize", buttonSize).toInt();
		settings.endGroup();

		for (int row = 0; row < buttons.size(); ++row)
		{
			for (int col = 0; col < buttons[row].size(); ++col)
			{
				gridLayout->removeWidget(buttons[row][col]);
				delete buttons[row][col];
			}
		}
		buttons.clear();
		mines.clear();
		cellStates.clear();

		buttons.resize(fieldHeight);
		mines.resize(fieldHeight);
		cellStates.resize(fieldHeight);

		for (int row = 0; row < fieldHeight; ++row)
		{
			buttons[row].resize(fieldWidth);
			mines[row].resize(fieldWidth);
			cellStates[row].resize(fieldWidth);
			for (int col = 0; col < fieldWidth; ++col)
			{
				CellButton *button = new CellButton();
				button->setFixedSize(buttonSize, buttonSize);
				button->setPosition(row, col);
				buttons[row][col] = button;
				gridLayout->addWidget(button, row + 1, col);
				cellStates[row][col] = 0;
			}
		}

		int size = settings.beginReadArray("Cells");
		for (int i = 0; i < size; ++i)
		{
			settings.setArrayIndex(i);
			int row = settings.value("row").toInt();
			int col = settings.value("col").toInt();
			mines[row][col] = settings.value("mine").toInt();
			QString buttonText = settings.value("text").toString();
			buttons[row][col]->setText(buttonText);
			buttons[row][col]->setEnabled(settings.value("enabled").toBool());

			buttons[row][col]->setStyleSheet(settings.value("color").toString());
		}
		settings.endArray();

		if (gameOver)
		{
			for (int r = 0; r < fieldHeight; ++r)
			{
				for (int c = 0; c < fieldWidth; ++c)
				{
					buttons[r][c]->setEnabled(false);
				}
			}
		}

		if (dbgMode && minesGenerated)
		{
			createDebugWindow();
		}

		updateButtonHandlers();
	}
	else
	{
		showHelpMessage();
		recreateField();
	}

	updateInterfaceTexts();
}

void MainWindow::updateMineCounter()
{
	int remainingMines = std::max(mineCount - flagsPlaced, 0);
	if (currentLanguage == Russian)
	{
		mineCounterLabel->setText("Количество оставшихся мин: " + QString::number(remainingMines));
	}
	else
	{
		mineCounterLabel->setText("Remaining Mines: " + QString::number(remainingMines));
	}
}

void MainWindow::showHelpMessage()
{
	QString message;
	if (currentLanguage == Russian)
	{
		message =
			"Внимание!\n\n"
			"Правила игры и обозначения в «Сапёре»:\n\n"
			"М - на этой клетке находится мина.\n"
			"Красная М - последняя открытая клетка оказалась с миной.\n"
			"F - флаг для обозначения предполагаемой мины (ставится после первого нажатия левой кнопкой мыши).\n"
			"? - появляется после следующего нажатия левой кнопкой мыши на клетку с флагом (F), чтобы обозначить "
			"сомнение.\n"
			"Цифра - показывает количество мин в соседних клетках.\n\n"
			"Особенности управления:\n\n"
			"Открытие клеток: осуществляется правой кнопкой мыши.\n"
			"Средняя кнопка мыши: если нажать на открытую клетку, вокруг которой количество флажков равно числу, "
			"указанному на этой клетке, то все соседние клетки откроются автоматически. Это ускоряет игру, если вы "
			"уверены в правильности своих пометок флажками.\n"
			"Игра поддерживает автосохранение, поэтому вы можете не переживать при закрытии программы — ваш прогресс "
			"сохранится.\n\n"
			"Кнопки управления:\n\n"
			"Новая игра: начнет новую игру с текущими параметрами.\n"
			"Настройки: выберите размер поля и количество мин для нового игрового поля.\n"
			"Левша: меняет управление правой и левой кнопками мыши.\n"
			"Больше/меньше: увеличивает или уменьшает все элементы приложения.\n"
			"En/Рус: переключает язык интерфейса между Русским и English.\n"
			"Количество оставшихся мин отображает общее число мин, уменьшенное на количество поставленных флагов (F).";
	}
	else
	{
		message =
			"Attention!\n\n"
			"Rules and symbols in Minesweeper:\n\n"
			"M - there is a mine on this cell.\n"
			"Red M - the last opened cell turned out to be a mine.\n"
			"F - a flag to mark a suspected mine (set after the first left mouse button click).\n"
			"? - appears after the next left mouse button click on a cell with a flag (F) to indicate doubt.\n"
			"Number - shows the number of mines in neighboring cells.\n\n"
			"Control features:\n\n"
			"Opening cells: done with the right mouse button.\n"
			"Middle mouse button: if you click on an open cell, around which the number of flags equals the number "
			"indicated on this cell, all neighboring cells will open automatically. This speeds up the game if you are "
			"sure about your flag markings.\n"
			"The game supports autosave, so you don't have to worry when closing the program — your progress will be "
			"saved.\n\n"
			"Control buttons:\n\n"
			"New Game: starts a new game with the current parameters.\n"
			"Settings: choose the field size and the number of mines for a new game field.\n"
			"Left-handed: changes the control of the right and left mouse buttons.\n"
			"Bigger/Smaller: increases or decreases all application elements.\n"
			"En/Рус: switches the interface language between Russian and English.\n"
			"The number of remaining mines shows the total number of mines, reduced by the number of placed flags (F).";
	}

	QMessageBox::information(this, currentLanguage == Russian ? "Помощь" : "Help", message);
}
