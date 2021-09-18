
#include "mooned_local.h"

#include "events.h"

#include "ui_contest.h"
#include "consolewindow.h"

static bool s_commandsAdded;

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

static void Con_Clear_f()
{
	s_text.clear();
	assert( g_pConsoleWindow ); // Should not be fired before init
	g_pConsoleWindow->Update();
}

static void Con_RegisterCommands()
{
	Cmd_AddCommand( "clear", Con_Clear_f, "Clear the console buffer." );
}

//=================================================================================================

ConsoleWindow::ConsoleWindow( QWidget *parent )
	: QWidget( parent )
	, ui( new Ui::ConsoleWindow )
{
	ui->setupUi( this );

	connect( ui->submitButton, &QPushButton::clicked, this, &ConsoleWindow::SubmitText );
	connect( ui->lineEdit, &QLineEdit::returnPressed, this, &ConsoleWindow::SubmitText );

	if ( !s_commandsAdded )
	{
		Con_RegisterCommands();
	}

	Update();
}

ConsoleWindow::~ConsoleWindow()
{
	delete ui;
}

//=================================================================================================

void ConsoleWindow::SubmitText()
{
	QByteArray byteArray = ui->lineEdit->text().toUtf8().constData();
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
