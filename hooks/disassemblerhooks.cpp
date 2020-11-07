#include "disassemblerhooks.h"
#include "../dialogs/tabledialog/tabledialog.h"
#include "../dialogs/aboutdialog/aboutdialog.h"
#include "../dialogs/problemsdialog/problemsdialog.h"
#include "../dialogs/settingsdialog/settingsdialog.h"
#include "../dialogs/analyzerdialog/analyzerdialog.h"
#include "../dialogs/loaderdialog/loaderdialog.h"
#include "../dialogs/referencesdialog/referencesdialog.h"
#include "../dialogs/gotodialog/gotodialog.h"
#include "../dialogs/devdialog/devdialog.h"
#include "../dialogs/databasedialog/databasedialog.h"
#include "../widgets/docks/outputdock/outputdock.h"
#include "../widgets/disassemblerview.h"
#include "../widgets/welcomewidget.h"
#include "../models/dev/blocklistmodel.h"
#include "../models/listingitemmodel.h"
#include "../redasmsettings.h"
#include "../redasmfonts.h"
#include <QMessageBox>
#include <QApplication>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QToolBar>
#include <QMenuBar>
#include <QDebug>
#include <QMenu>
#include <unordered_map>
#include <rdapi/rdapi.h>

#define MAX_FUNCTION_NAME 50

DisassemblerHooks DisassemblerHooks::m_instance;

DisassemblerHooks::DisassemblerHooks(QObject* parent): QObject(parent) { }

void DisassemblerHooks::initialize(QMainWindow* mainwindow)
{
    DisassemblerHooks::m_instance.m_mainwindow = mainwindow;
    DisassemblerHooks::m_instance.hook();
}

DisassemblerHooks* DisassemblerHooks::instance() { return &m_instance; }
QMainWindow* DisassemblerHooks::mainWindow() const { return m_mainwindow; }
void DisassemblerHooks::log(const QString& s) { this->outputDock()->log(s); }
void DisassemblerHooks::clearLog() { this->outputDock()->clear(); }

void DisassemblerHooks::resetLayout()
{

}

void DisassemblerHooks::open()
{
    QString s = QFileDialog::getOpenFileName(m_mainwindow, "Disassemble file...");
    if(!s.isEmpty()) this->load(s);
}

void DisassemblerHooks::close() { this->close(true); }

void DisassemblerHooks::save()
{

}

void DisassemblerHooks::saveAs()
{

}

void DisassemblerHooks::settings()
{
    SettingsDialog dlgsettings(m_mainwindow);
    dlgsettings.exec();
}

void DisassemblerHooks::about()
{
    AboutDialog dlgabout(m_mainwindow);
    dlgabout.exec();
}

void DisassemblerHooks::exit() { qApp->exit(); }

void DisassemblerHooks::showReferences(ICommand* command, rd_address address)
{
    RDDocument* doc = RDContext_GetDocument(command->context().get());

    RDSymbol symbol;
    if(!RDDocument_GetSymbolByAddress(doc, address, &symbol)) return;

    const RDNet* net = RDContext_GetNet(command->context().get());

    if(!RDNet_GetReferences(net, symbol.address, nullptr))
    {
        QMessageBox::information(nullptr, "No References", QString("There are no references to %1 ").arg(RDDocument_GetSymbolName(doc, symbol.address)));
        return;
    }

    ReferencesDialog dlgreferences(command, &symbol, m_mainwindow);
    dlgreferences.exec();
}

void DisassemblerHooks::showGoto(ICommand* command)
{
    GotoDialog dlggoto(command);
    dlggoto.exec();
}

void DisassemblerHooks::showDeveloperTools()
{
    if(!m_devdialog) m_devdialog = new DevDialog(m_mainwindow);
    m_devdialog->setCommand(m_activecommandtab->command());
    m_devdialog->show();
}

void DisassemblerHooks::showDatabase()
{
    DatabaseDialog dbdialog(m_mainwindow);
    dbdialog.exec();
}

void DisassemblerHooks::showProblems() { ProblemsDialog dlgproblems(this->activeContext(), m_mainwindow); dlgproblems.exec(); }
void DisassemblerHooks::focusOn(QWidget* w) { if(m_disassemblerview) m_disassemblerview->focusOn(w); }

ICommandTab* DisassemblerHooks::activeCommandTab() const
{
    if(!m_activecommandtab) m_activecommandtab = m_disassemblerview->showListing();
    return m_activecommandtab;
}

ICommand* DisassemblerHooks::activeCommand() const { return this->activeCommandTab()->command(); }
const RDContextPtr& DisassemblerHooks::activeContext() const { return this->activeCommand()->context(); }

void DisassemblerHooks::undock(QDockWidget* dw)
{
    m_mainwindow->removeDockWidget(dw);
    dw->deleteLater();
}

void DisassemblerHooks::onToolBarActionTriggered(QAction* action)
{
    if(!m_activecommandtab) return;

    int idx = m_toolbar->actions().indexOf(action);

    switch(idx)
    {
        case 2: m_activecommandtab->command()->goBack(); break;
        case 3: m_activecommandtab->command()->goForward(); break;
        case 4: this->showGoto(m_activecommandtab->command()); break;

        case 5: {
            auto* tabletab = dynamic_cast<ITableTab*>(m_disassemblerview->currentWidget());
            if(tabletab) tabletab->toggleFilter();
            break;
        }

        default: break;
    }
}

void DisassemblerHooks::onWindowActionTriggered(QAction* action)
{
    int idx = m_mnuwindow->actions().indexOf(action);

    switch(idx)
    {
        case 0: {
            ICommandTab* tab = m_activecommandtab;
            if(!tab) tab = m_disassemblerview->showListing();
            this->focusOn(dynamic_cast<QWidget*>(tab));
            break;
        }

        case 1: m_disassemblerview->showSegments();  break;
        case 2: m_disassemblerview->showFunctions(); break;
        case 3: m_disassemblerview->showExports();   break;
        case 4: m_disassemblerview->showImports();   break;
        case 5: m_disassemblerview->showStrings();   break;
        default: break;
    }
}

void DisassemblerHooks::statusAddress(const ICommand* command) const
{
    if(RDContext_IsBusy(command->context().get())) return;

    RDDocument* doc = RDContext_GetDocument(command->context().get());

    RDDocumentItem item;
    if(!command->getCurrentItem(&item)) return;

    RDLoader* ldr = RDContext_GetLoader(command->context().get());

    RDSegment segment;
    bool hassegment = RDDocument_GetSegmentAddress(doc, item.address, &segment);

    RDLocation functionstart = RDDocument_GetFunctionStart(doc, item.address);
    RDLocation offset = RD_Offset(ldr, item.address);

    QString segm = hassegment ? segment.name : "UNKNOWN",
            offs = hassegment && offset.valid ? RD_ToHexAuto(offset.value) : "UNKNOWN",
            addr = RD_ToHexAuto(item.address);

    QString s = QString::fromWCharArray(L"<b>Address: </b>%1\u00A0\u00A0").arg(addr);
    s += QString::fromWCharArray(L"<b>Offset: </b>%1\u00A0\u00A0").arg(offs);
    s += QString::fromWCharArray(L"<b>Segment: </b>%1\u00A0\u00A0").arg(segm);

    const char* functionstartname = RDDocument_GetSymbolName(doc, functionstart.value);

    if(functionstart.valid && functionstartname)
    {
        QString func = functionstartname;

        if(func.size() > MAX_FUNCTION_NAME)
            func = func.left(MAX_FUNCTION_NAME) + "..."; // Elide long function names

        if(item.address > functionstart.value)
            func += "+" + QString::fromUtf8(RD_ToHexBits(item.address - functionstart.value, 8, false));
        else if(item.address < functionstart.value)
            func += QString::number(static_cast<std::make_signed<size_t>::type>(item.address - functionstart.value));

        s = QString::fromWCharArray(L"<b>Function: </b>%1\u00A0\u00A0").arg(func.toHtmlEscaped()) + s;
    }

    RD_Status(qUtf8Printable(s));
}

void DisassemblerHooks::adjustActions()
{
    QMenu* menu = static_cast<QMenu*>(this->sender());
    std::unordered_map<int, QAction*> actions;

    for(QAction* action : menu->actions())
    {
        QVariant data = action->data();
        if(!data.isNull()) actions[data.toInt()] = action;
    }

    ICommand* command = dynamic_cast<ICommand*>(menu->parentWidget());
    RDDocumentItem item;
    if(!command->getCurrentItem(&item)) return;

    RDDocument* doc = RDContext_GetDocument(command->context().get());

    actions[DisassemblerHooks::Action_Back]->setVisible(command->canGoBack());
    actions[DisassemblerHooks::Action_Forward]->setVisible(command->canGoForward());
    actions[DisassemblerHooks::Action_Copy]->setVisible(command->hasSelection());
    actions[DisassemblerHooks::Action_Goto]->setVisible(!RDContext_IsBusy(command->context().get()));

    RDSegment itemsegment, symbolsegment;
    RDSymbol symbol;

    if(!command->getCurrentSymbol(&symbol))
    {
        bool hassymbolsegment = RDDocument_GetSegmentAddress(doc, item.address, &symbolsegment);
        RDLocation funcstart = RDDocument_GetFunctionStart(doc, item.address);
        const char* funcname = funcstart.valid ? RDDocument_GetSymbolName(doc, funcstart.value) : nullptr;

        actions[DisassemblerHooks::Action_Rename]->setVisible(false);
        actions[DisassemblerHooks::Action_XRefs]->setVisible(false);
        actions[DisassemblerHooks::Action_Follow]->setVisible(false);
        actions[DisassemblerHooks::Action_FollowPointerHexDump]->setVisible(false);

        if(!RDContext_IsBusy(command->context().get()))
        {
            bool ok = false;
            RDSegment currentsegment;
            rd_address currentaddress = command->currentWord().toUInt(&ok, 16);
            bool hascurrentsegment = ok ? RDDocument_GetSegmentAddress(doc, currentaddress, &currentsegment) : false;

            actions[DisassemblerHooks::Action_CreateFunction]->setVisible(hascurrentsegment && HAS_FLAG(&currentsegment, SegmentFlags_Code));

            if(hascurrentsegment)
                actions[DisassemblerHooks::Action_CreateFunction]->setText(QString("Create Function @ %1").arg(RD_ToHexAuto(currentaddress)));
        }
        else
            actions[DisassemblerHooks::Action_CreateFunction]->setVisible(false);

        if(funcname)
            actions[DisassemblerHooks::Action_CallGraph]->setText(QString("Callgraph %1").arg(funcname));

        actions[DisassemblerHooks::Action_CallGraph]->setVisible(funcname && hassymbolsegment && HAS_FLAG(&symbolsegment, SegmentFlags_Code));
        actions[DisassemblerHooks::Action_HexDumpFunction]->setVisible(funcname);
        actions[DisassemblerHooks::Action_HexDump]->setVisible(true);
        return;
    }

    bool hasitemsegment = RDDocument_GetSegmentAddress(doc, item.address, &itemsegment);
    const char* symbolname = RDDocument_GetSymbolName(doc, symbol.address);
    bool hassymbolsegment = RDDocument_GetSegmentAddress(doc, symbol.address, &symbolsegment);

    actions[DisassemblerHooks::Action_CreateFunction]->setText(QString("Create Function @ %1").arg(RD_ToHexAuto(symbol.address)));

    actions[DisassemblerHooks::Action_CreateFunction]->setVisible(!RDContext_IsBusy(command->context().get()) && (hassymbolsegment && HAS_FLAG(&symbolsegment,SegmentFlags_Code)) &&
                                                                    (HAS_FLAG(&symbol, SymbolFlags_Weak) && !IS_TYPE(&symbol, SymbolType_Function)));


    actions[DisassemblerHooks::Action_FollowPointerHexDump]->setText(QString("Follow %1 pointer in Hex Dump").arg(symbolname));
    actions[DisassemblerHooks::Action_FollowPointerHexDump]->setVisible(HAS_FLAG(&symbol, SymbolFlags_Pointer));

    actions[DisassemblerHooks::Action_XRefs]->setText(QString("Cross Reference %1").arg(symbolname));
    actions[DisassemblerHooks::Action_XRefs]->setVisible(!RDContext_IsBusy(command->context().get()));

    actions[DisassemblerHooks::Action_Rename]->setText(QString("Rename %1").arg(symbolname));
    actions[DisassemblerHooks::Action_Rename]->setVisible(!RDContext_IsBusy(command->context().get()) && HAS_FLAG(&symbol, SymbolFlags_Weak));

    actions[DisassemblerHooks::Action_CallGraph]->setText(QString("Callgraph %1").arg(symbolname));
    actions[DisassemblerHooks::Action_CallGraph]->setVisible(!RDContext_IsBusy(command->context().get()) && IS_TYPE(&symbol, SymbolType_Function));

    actions[DisassemblerHooks::Action_Follow]->setText(QString("Follow %1").arg(symbolname));
    actions[DisassemblerHooks::Action_Follow]->setVisible(IS_TYPE(&symbol, SymbolType_Label));

    actions[DisassemblerHooks::Action_Comment]->setVisible(!RDContext_IsBusy(command->context().get()) && IS_TYPE(&item, DocumentItemType_Instruction));

    actions[DisassemblerHooks::Action_HexDump]->setVisible(hassymbolsegment && HAS_FLAG(&symbolsegment, SegmentFlags_Bss));
    actions[DisassemblerHooks::Action_HexDumpFunction]->setVisible(hasitemsegment && !HAS_FLAG(&itemsegment, SegmentFlags_Bss) && HAS_FLAG(&itemsegment, SegmentFlags_Code));
}

void DisassemblerHooks::loadDisassemblerView(const RDContextPtr& ctx)
{
    this->close(false);

    m_disassemblerview = new DisassemblerView(ctx);
    this->replaceWidget(m_disassemblerview);
}

void DisassemblerHooks::hook()
{
    m_lblstatusicon = m_mainwindow->findChild<QLabel*>(HOOK_STATUS_ICON);
    m_pbrenderer = m_mainwindow->findChild<QPushButton*>(HOOK_RENDERER);
    m_pbproblems = m_mainwindow->findChild<QPushButton*>(HOOK_PROBLEMS);
    m_mnuwindow = m_mainwindow->findChild<QMenu*>(HOOK_MENU_WINDOW);
    m_toolbar = m_mainwindow->findChild<QToolBar*>(HOOK_TOOLBAR);
    this->showWelcome();

    QAction* act = m_mainwindow->findChild<QAction*>(HOOK_ACTION_DATABASE);
    connect(act, &QAction::triggered, this, [&]() { this->showDatabase(); });

    connect(m_pbproblems, &QPushButton::clicked, this, [&]() { this->showProblems(); });
    connect(m_toolbar, &QToolBar::actionTriggered, this, &DisassemblerHooks::onToolBarActionTriggered);
    connect(m_mnuwindow, &QMenu::triggered, this, &DisassemblerHooks::onWindowActionTriggered);

    connect(m_pbrenderer, &QPushButton::clicked, this, [&]() {
        const RDContextPtr& ctx = this->activeContext();
        RDContext_SetFlags(ctx.get(), ContextFlags_ShowRDIL, !RDContext_HasFlag(ctx.get(), ContextFlags_ShowRDIL));
        this->checkListingMode();
    });

    this->dock(new OutputDock(m_mainwindow), Qt::BottomDockWidgetArea);
    this->enableCommands(nullptr);
    this->enableViewCommands(false);
    this->loadRecents();
}

void DisassemblerHooks::showLoaders(const QString& filepath, RDBuffer* buffer)
{
    RDContextPtr ctx(RDContext_Create(), RDObjectDeleter());
    QByteArray rawfilepath = filepath.toUtf8();
    RDLoaderRequest req = { rawfilepath.data(), buffer, { } };

    LoaderDialog dlgloader(ctx, &req, m_mainwindow);

    if(dlgloader.exec() != LoaderDialog::Accepted)
    {
        RDObject_Free(buffer);
        return;
    }

    this->clearOutput();

    req.buildparams = dlgloader.buildRequest();
    RDDisassembler* disassembler = RDContext_BuildDisassembler(ctx.get(), &req, dlgloader.selectedLoaderEntry(), dlgloader.selectedAssemblerEntry());
    if(!disassembler) return;

    const RDLoader* loader = RDContext_GetLoader(ctx.get());
    const RDAssembler* assembler = RDContext_GetAssembler(ctx.get());
    rd_log(qUtf8Printable(QString("Selected loader '%1' with '%2' assembler").arg(RDLoader_GetName(loader), RDAssembler_GetName(assembler))));

    AnalyzerDialog dlganalyzer(ctx, m_mainwindow);
    if(dlganalyzer.exec() != AnalyzerDialog::Accepted) return;

    m_fileinfo = QFileInfo(filepath);
    this->loadDisassemblerView(ctx);
}

void DisassemblerHooks::showWelcome() { this->replaceWidget(new WelcomeWidget()); }

QMenu* DisassemblerHooks::createActions(ICommand* command)
{
    QMenu* contextmenu = new QMenu(command->widget());
    std::unordered_map<int, QAction*> actions;

    actions[DisassemblerHooks::Action_Rename] = contextmenu->addAction("Rename", this, [&, command]() {
        RDSymbol symbol;
        if(!command->getCurrentSymbol(&symbol)) return;

        RDDocument* doc = RDContext_GetDocument(command->context().get());
        const char* symbolname = RDDocument_GetSymbolName(doc, symbol.address);
        if(!symbolname) return;

        bool ok = false;
        QString res = QInputDialog::getText(command->widget(),
                                            "Rename @ " + QString::fromStdString(rd_tohexauto(symbol.address)),
                                            "Symbol name:", QLineEdit::Normal, symbolname, &ok);

        if(!ok) return;
        RDDocument_Rename(doc, symbol.address, qUtf8Printable(res));
    }, QKeySequence(Qt::Key_N));

    actions[DisassemblerHooks::Action_Comment] = contextmenu->addAction("Comment", this, [&, command]() {
        RDDocumentItem item;
        if(!command->getCurrentItem(&item)) return;

        RDDocument* doc = RDContext_GetDocument(command->context().get());

        bool ok = false;
        QString res = QInputDialog::getMultiLineText(command->widget(),
                                                     "Comment @ " + QString::fromStdString(rd_tohexauto(item.address)),
                                                     "Insert a comment (leave blank to remove):",
                                                     RDDocument_GetComments(doc, item.address, "\n"), &ok);

        if(!ok) return;
        RDDocument_Comment(doc, item.address, qUtf8Printable(res));
    }, QKeySequence(Qt::Key_Semicolon));

    contextmenu->addSeparator();

    actions[DisassemblerHooks::Action_XRefs] = contextmenu->addAction("Cross References", this, [&, command]() {
        RDSymbol symbol;
        if(!command->getCurrentSymbol(&symbol)) return;
        this->showReferences(command, symbol.address);
    }, QKeySequence(Qt::Key_X));

    actions[DisassemblerHooks::Action_Follow] = contextmenu->addAction("Follow", this, [command]() {
        RDSymbol symbol;
        if(!command->getCurrentSymbol(&symbol)) return false;
        return command->goToAddress(symbol.address);
    });

    actions[DisassemblerHooks::Action_FollowPointerHexDump] = contextmenu->addAction("Follow pointer in Hex Dump", this, [&, command]() {
    });

    actions[DisassemblerHooks::Action_Goto] = contextmenu->addAction("Goto...", this, [&, command]() {
        this->showGoto(command);
    }, QKeySequence(Qt::Key_G));

    actions[DisassemblerHooks::Action_CallGraph] = contextmenu->addAction("Call Graph", this, [&, command]() {

    }, QKeySequence(Qt::CTRL + Qt::Key_G));

    contextmenu->addSeparator();

    actions[DisassemblerHooks::Action_HexDump] = contextmenu->addAction("Show Hex Dump", this, [&, command]() {
    }, QKeySequence(Qt::CTRL + Qt::Key_H));

    actions[DisassemblerHooks::Action_HexDumpFunction] = contextmenu->addAction("Hex Dump Function", this, [&, command]() {
        RDDocumentItem item;
        if(!command->getCurrentItem(&item)) return;

        RDSymbol symbol;
        const char* hexdump = RDDisassembler_FunctionHexDump(command->disassembler(), item.address, &symbol);
        if(!hexdump) return;

        RDDocument* doc = RDContext_GetDocument(command->context().get());
        const char* name = RDDocument_GetSymbolName(doc, symbol.address);
        RD_Log(qUtf8Printable(QString("%1: %2").arg(name, hexdump)));
    });

    actions[DisassemblerHooks::Action_CreateFunction] = contextmenu->addAction("Create Function", this, [&, command]() {
        RDSymbol symbol;
        if(!command->getCurrentSymbol(&symbol)) {
            rd_log("Cannot create function @ " + rd_tohex(symbol.address));
            return;
        }

        m_worker = std::async([&]() { RDDisassembler_CreateFunction(command->disassembler(), symbol.address, nullptr); });
    }, QKeySequence(Qt::SHIFT + Qt::Key_C));

    contextmenu->addSeparator();
    actions[DisassemblerHooks::Action_Back] = contextmenu->addAction("Back", this, [command]() { command->goBack(); }, QKeySequence(Qt::CTRL + Qt::Key_Left));
    actions[DisassemblerHooks::Action_Forward] = contextmenu->addAction("Forward", this, [command]() { command->goForward(); }, QKeySequence(Qt::CTRL + Qt::Key_Right));
    contextmenu->addSeparator();
    actions[DisassemblerHooks::Action_Copy] = contextmenu->addAction("Copy", this, [command]() { command->copy(); }, QKeySequence(QKeySequence::Copy));

    for(auto& [type, action] : actions) action->setData(type);

    command->widget()->addAction(actions[DisassemblerHooks::Action_Rename]);
    command->widget()->addAction(actions[DisassemblerHooks::Action_XRefs]);
    command->widget()->addAction(actions[DisassemblerHooks::Action_Comment]);
    command->widget()->addAction(actions[DisassemblerHooks::Action_Goto]);
    command->widget()->addAction(actions[DisassemblerHooks::Action_CallGraph]);
    command->widget()->addAction(actions[DisassemblerHooks::Action_HexDump]);
    command->widget()->addAction(actions[DisassemblerHooks::Action_CreateFunction]);
    command->widget()->addAction(actions[DisassemblerHooks::Action_Back]);
    command->widget()->addAction(actions[DisassemblerHooks::Action_Forward]);
    command->widget()->addAction(actions[DisassemblerHooks::Action_Copy]);

    connect(contextmenu, &QMenu::aboutToShow, this, &DisassemblerHooks::adjustActions);
    return contextmenu;
}

void DisassemblerHooks::setActiveCommandTab(ICommandTab* commandtab) { m_activecommandtab = commandtab; }

void DisassemblerHooks::loadRecents()
{
    QAction* actrecents = m_mainwindow->findChild<QAction*>(HOOK_ACTION_RECENT_FILES);
    if(!actrecents) return;

    REDasmSettings settings;
    QStringList recents = settings.recentFiles();
    actrecents->setEnabled(!recents.empty());

    QMenu* recentsmenu = new QMenu(m_mainwindow);

    for(int i = 0; i < MAX_RECENT_FILES; i++)
    {
        if(i >= recents.length())
        {
            QAction* action = recentsmenu->addAction(QString());
            action->setVisible(false);
            continue;
        }

        if(!QFileInfo(recents[i]).exists()) continue;

        QAction* action = recentsmenu->addAction(QString("%1 - %2").arg(i).arg(recents[i]));
        action->setData(recents[i]);

        connect(action, &QAction::triggered, this, [=]() {
            this->load(action->data().toString());
        });
    }

    if(actrecents->menu()) actrecents->menu()->deleteLater();
    actrecents->setMenu(recentsmenu);
}

void DisassemblerHooks::load(const QString& filepath)
{
    QFileInfo fi(filepath);
    QDir::setCurrent(fi.path());

    REDasmSettings settings;
    settings.updateRecentFiles(filepath);
    this->loadRecents();

    if(this->openDatabase(filepath)) return;

    RDBuffer* buffer = RDBuffer_CreateFromFile(qUtf8Printable(filepath));
    if(buffer && RDBuffer_Size(buffer)) this->showLoaders(filepath, buffer);
    else if(buffer) RDObject_Free(buffer);
}

void DisassemblerHooks::dock(QWidget* w, Qt::DockWidgetArea area)
{
    QDockWidget* dw = dynamic_cast<QDockWidget*>(w);

    if(!dw)
    {
        dw = new QDockWidget(m_mainwindow);
        w->setParent(dw); // Take ownership
        dw->setWindowTitle(w->windowTitle());
        dw->setWidget(w);
    }

    m_mainwindow->addDockWidget(area, dw);
}

OutputDock* DisassemblerHooks::outputDock() const { return m_mainwindow->findChild<OutputDock*>(QString(), Qt::FindDirectChildrenOnly); }

void DisassemblerHooks::checkListingMode()
{
    if(RDContext_HasFlag(this->activeContext().get(), ContextFlags_ShowRDIL)) m_pbrenderer->setText("RDIL");
    else m_pbrenderer->setText("Listing");
}

void DisassemblerHooks::close(bool showwelcome)
{
    this->enableViewCommands(false);
    this->enableCommands(nullptr);

    if(showwelcome) this->showWelcome(); // Replaces central widget, if any
    else if(m_disassemblerview) m_disassemblerview->deleteLater();
    m_disassemblerview = nullptr;
}

void DisassemblerHooks::replaceWidget(QWidget* w)
{
    QWidget* oldw = m_mainwindow->centralWidget();

    if(oldw)
    {
        connect(oldw, &QWidget::destroyed, this, [w, this]() {
            m_mainwindow->setCentralWidget(w);
        });

        oldw->deleteLater();
    }
    else
        m_mainwindow->setCentralWidget(w);
}

void DisassemblerHooks::clearOutput()
{
    OutputDock* outputpart = this->outputDock();
    if(outputpart) outputpart->clear();
}

void DisassemblerHooks::enableMenu(QMenu* menu, bool enable)
{
    auto actions = menu->actions();
    std::for_each(actions.begin(), actions.end(), [enable](QAction* a) { a->setEnabled(enable); });

    if(dynamic_cast<QMenuBar*>(menu->parentWidget())) menu->menuAction()->setVisible(enable);
    menu->setEnabled(enable);
}

bool DisassemblerHooks::openDatabase(const QString& filepath)
{
    return false;
}

void DisassemblerHooks::enableViewCommands(bool enable)
{
    this->enableMenu(m_mnuwindow, enable);

    auto actions = m_toolbar->actions();
    actions[1]->setEnabled(enable);

    QAction* act = m_mainwindow->findChild<QAction*>(HOOK_ACTION_SAVE_AS);
    act->setEnabled(enable);

    act = m_mainwindow->findChild<QAction*>(HOOK_ACTION_CLOSE);
    act->setEnabled(enable);
}

void DisassemblerHooks::showMessage(const QString& title, const QString& msg, size_t icon)
{
    QMessageBox msgbox(m_mainwindow);
    msgbox.setWindowTitle(title);
    msgbox.setText(msg);
    msgbox.setIcon(static_cast<QMessageBox::Icon>(icon));
    msgbox.exec();
}

void DisassemblerHooks::updateViewWidgets(bool busy)
{
    if(!m_disassemblerview)
    {
        m_toolbar->actions()[4]->setEnabled(false);
        m_lblstatusicon->setVisible(false);
        m_pbproblems->setVisible(false);
        m_mainwindow->setWindowTitle(QString());
        return;
    }

    if(busy)
    {
        m_mainwindow->setWindowTitle(QString("%1 (Working)").arg(m_fileinfo.fileName()));
        m_lblstatusicon->setStyleSheet("color: red;");
    }
    else
    {
        m_mainwindow->setWindowTitle(m_fileinfo.fileName());
        m_lblstatusicon->setStyleSheet("color: green;");
    }

    m_toolbar->actions()[4]->setEnabled(!busy); // Goto
    m_lblstatusicon->setVisible(true);

    if(this->activeContext())
    {
        m_pbproblems->setVisible(!busy && RDContext_HasProblems(this->activeContext().get()));
        m_pbproblems->setText(QString::number(RDContext_GetProblemsCount(this->activeContext().get())) + " problem(s)");
    }
    else
        m_pbproblems->setVisible(false);
}

void DisassemblerHooks::enableCommands(QWidget* w)
{
    QAction* act = m_mainwindow->findChild<QAction*>(HOOK_ACTION_DEVTOOLS);
    auto actions = m_toolbar->actions();

    if(!w)
    {
        for(int i = 2; i < actions.size(); i++)
            actions[i]->setVisible(false);

        act->setVisible(false);
        m_pbrenderer->setVisible(false);
        return;
    }

    auto* commandtab = dynamic_cast<ICommandTab*>(w);
    this->checkListingMode();

    m_pbrenderer->setVisible(commandtab);
    act->setVisible(commandtab);

    actions[2]->setVisible(commandtab); // Back
    actions[3]->setVisible(commandtab); // Forward
    actions[4]->setVisible(commandtab); // Goto

    auto* tabletab = dynamic_cast<ITableTab*>(w);
    actions[5]->setVisible(tabletab); // Filter
}

void DisassemblerHooks::updateCommandStates(QWidget* w) const
{
    auto actions = m_toolbar->actions();

    if(auto* commandtab = dynamic_cast<ICommandTab*>(w))
    {
        actions[2]->setEnabled(commandtab->command()->canGoBack());
        actions[3]->setEnabled(commandtab->command()->canGoForward());
    }
}
