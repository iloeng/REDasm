#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "widgets/disassemblerview/disassemblerview.h"
#include "dialogs/manualloaddialog.h"
#include "dialogs/databasedialog.h"
#include "dialogs/settingsdialog.h"
#include "dialogs/aboutdialog.h"
#include "redasmsettings.h"
#include "themeprovider.h"
#include <QtCore>
#include <QtGui>
#include <QtWidgets>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    REDasm::setStatusCallback([this](std::string s) {
        QMetaObject::invokeMethod(m_lblstatus, "setText", Qt::QueuedConnection, Q_ARG(QString, S_TO_QS(s)));
    });

    REDasm::init(QDir::currentPath().toStdString());
    ui->setupUi(this);

    ui->tbAbout->setIcon(THEME_ICON("about"));
    ui->tbDatabase->setIcon(THEME_ICON("database"));
    ui->tbOpen->setIcon(THEME_ICON("open"));
    ui->tbSave->setIcon(THEME_ICON("save"));

    m_lblstatus = new QLabel(this);

    m_pbstatus = new QPushButton(this);
    m_pbstatus->setFlat(true);
    m_pbstatus->setFixedWidth(ui->statusBar->height() * 0.8);
    m_pbstatus->setFixedHeight(ui->statusBar->height() * 0.8);
    m_pbstatus->setText(QString::fromWCharArray(L"\u25cf"));
    m_pbstatus->setVisible(false);

    ui->statusBar->addPermanentWidget(m_lblstatus, 1);
    ui->statusBar->addPermanentWidget(m_pbstatus);

    this->loadGeometry();
    this->setAcceptDrops(true);

    connect(ui->action_Open, &QAction::triggered, this, &MainWindow::onOpenClicked);
    connect(ui->action_Settings, &QAction::triggered, this, &MainWindow::onSettingsClicked);
    connect(ui->action_Database, &QAction::triggered, this, &MainWindow::onDatabaseClicked);
    connect(ui->action_About_REDasm, &QAction::triggered, this, &MainWindow::onAboutClicked);

    this->checkCommandLine();
    this->loadRecents();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::closeEvent(QCloseEvent *e)
{
    if(!m_buffer.empty())
    {
        QMessageBox msgbox(this);
        msgbox.setWindowTitle("Closing");
        msgbox.setText("Are you sure?");
        msgbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if(msgbox.exec() != QMessageBox::Yes)
        {
            e->ignore();
            return;
        }
    }

    REDasmSettings settings;
    settings.changeGeometry(this->saveGeometry());
    QWidget::closeEvent(e);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if(!e->mimeData()->hasUrls())
        return;

    e->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *e)
{
    if(!e->mimeData()->hasUrls())
        return;

    e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *e)
{
    const QMimeData* mimedata = e->mimeData();

    if(!mimedata->hasUrls())
        return;

      QList<QUrl> urllist = mimedata->urls();
      QString locfile = urllist.first().toLocalFile();
      QFileInfo fi(locfile);

      if(!fi.isFile())
          return;

      this->load(locfile);
      e->acceptProposedAction();
}

void MainWindow::onOpenClicked()
{
    QString s = QFileDialog::getOpenFileName(this, "Disassemble file...");

    if(s.isEmpty())
        return;

    this->load(s);
}

void MainWindow::onRecentFileClicked()
{
    QAction* sender = qobject_cast<QAction*>(this->sender());

    if(sender)
        this->load(sender->data().toString());
}

void MainWindow::onSettingsClicked()
{
    SettingsDialog sd(this);
    sd.exec();
}

void MainWindow::loadGeometry()
{
    REDasmSettings settings;

    if(settings.hasGeometry())
    {
        this->restoreGeometry(settings.geometry());
        return;
    }

    QRect position = this->frameGeometry();
    position.moveCenter(qApp->primaryScreen()->availableGeometry().center());
    this->move(position.topLeft());
}

void MainWindow::loadRecents()
{
    REDasmSettings settings;
    m_recents = settings.recentFiles();
    ui->action_Recent_Files->setEnabled(!m_recents.empty());

    QMenu* mnurecents = ui->action_Recent_Files->menu();

    if(!mnurecents)
    {
        mnurecents = new QMenu(this);
        ui->action_Recent_Files->setMenu(mnurecents);
    }
    else
        mnurecents->clear();

    for(int i = 0; i < MAX_RECENT_FILES; i++)
    {
        if(i >= m_recents.length())
        {
            QAction* action = mnurecents->addAction(QString());
            action->setVisible(false);
            continue;
        }

        QAction* action = mnurecents->addAction(QString("%1 - %2").arg(i).arg(m_recents[i]));
        action->setData(m_recents[i]);
        connect(action, &QAction::triggered, this, &MainWindow::onRecentFileClicked);
    }

}

void MainWindow::load(const QString& filepath)
{
    QFileInfo fi(filepath);
    QDir::setCurrent(fi.path());
    this->setWindowTitle(fi.fileName());

    m_buffer = REDasm::Buffer::fromFile(filepath.toStdString());

    if(!m_buffer.empty())
    {
        REDasmSettings settings;
        settings.updateRecentFiles(filepath);

        this->loadRecents();
        this->initDisassembler();
    }
}

void MainWindow::checkCommandLine()
{
    QStringList args = qApp->arguments();

    if(args.length() <= 1)
        return;

    args = args.mid(1); // Skip REDasm's path

    for(const QString& arg : args)
    {
        QFileInfo fileinfo(arg);

        if(!fileinfo.exists() || !fileinfo.isFile() || !fileinfo.isReadable())
            continue;

        this->load(arg);
        break;
    }
}

bool MainWindow::checkPlugins(REDasm::FormatPlugin** format, REDasm::AssemblerPlugin** assembler)
{
    *format = REDasm::getFormat(m_buffer);

    if((*format)->isBinary()) // Use manual loader
    {
        ManualLoadDialog dlgmanload(*format, this);

        if(dlgmanload.exec() != ManualLoadDialog::Accepted)
            return false;
    }

    *assembler = REDasm::getAssembler((*format)->assembler());

    if(!(*assembler))
    {
        QMessageBox::information(this, "Assembler not found", QString("Cannot find assembler '%1'").arg(QString::fromUtf8((*format)->assembler())));
        return false;
    }

    return true;
}

void MainWindow::initDisassembler()
{
    DisassemblerView *dv = new DisassemblerView(m_lblstatus, m_pbstatus, ui->stackView);
    REDasm::FormatPlugin* format = NULL;
    REDasm::AssemblerPlugin* assembler = NULL;

    if(!this->checkPlugins(&format, &assembler))
    {
        dv->deleteLater();
        return;
    }

    REDasm::Disassembler* disassembler = new REDasm::Disassembler(assembler, format);
    dv->setDisassembler(disassembler);
    ui->stackView->addWidget(dv);

    QWidget* oldwidget = static_cast<DisassemblerView*>(ui->stackView->widget(0));
    ui->stackView->removeWidget(oldwidget);
    oldwidget->deleteLater();
}

void MainWindow::onDatabaseClicked()
{
    DatabaseDialog dlgdatabase(this);
    dlgdatabase.exec();
}

void MainWindow::onAboutClicked()
{
    AboutDialog dlgabout(this);
    dlgabout.exec();
}
