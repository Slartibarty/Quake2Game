
#include "mooned_local.h"

#include "ui_contest.h"
#include "consolewindow.h"

// The only instance of the console window
ConsoleWindow *g_pConsoleWindow;

// So we can start logging immediately
static QString s_text;

void Con_Print( const char *msg )
{
	s_text.append( msg );
	if ( g_pConsoleWindow )
	{
		g_pConsoleWindow->Update();
	}
}

CON_COMMAND( clear, "Clear the console buffer.", 0 )
{
	s_text.clear();
	if ( g_pConsoleWindow )
	{
		g_pConsoleWindow->Update();
	}
}

//=================================================================================================

ConsoleWindow::ConsoleWindow( QWidget *parent )
	: Super( parent )
	, ui( new Ui::ConsoleWindow )
{
	ui->setupUi( this );

	connect( ui->submitButton, &QPushButton::clicked, this, &ConsoleWindow::SubmitText );
	connect( ui->lineEdit, &QLineEdit::returnPressed, this, &ConsoleWindow::SubmitText );

	Update();
}

ConsoleWindow::~ConsoleWindow()
{
}

//=================================================================================================

void ConsoleWindow::SubmitText()
{
	QByteArray byteArray = ui->lineEdit->text().toUtf8();
	const char *text = byteArray.constData();

	// backslash text are commands, else chat
	if ( text[0] == '\\' || text[0] == '/' )
	{
		// skip the >
		Cbuf_AddText( text + 1 );
	}
	else
	{
		// valid command
		Cbuf_AddText( text );
	}

	Cbuf_AddText( "\n" );

	Com_Printf( "] %s\n", text );

	Cbuf_Execute();				// HACK? Need to find a way into Qt's event loop

	ui->lineEdit->clear();
}

void ConsoleWindow::Update()
{
	ui->plainTextEdit->setPlainText( s_text );
}
