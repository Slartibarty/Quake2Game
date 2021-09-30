
#pragma once

void Con_Print( const char *msg );

QT_BEGIN_NAMESPACE
namespace Ui { class ConsoleWindow; }
QT_END_NAMESPACE

class ConsoleWindow final : public QWidget
{
	Q_OBJECT

private:
	using Super = QWidget;

	Ui::ConsoleWindow *ui;

public:
	explicit ConsoleWindow( QWidget *parent = nullptr );
	~ConsoleWindow() override;

	void SubmitText();
	void Update();
};

extern ConsoleWindow *g_pConsoleWindow;
