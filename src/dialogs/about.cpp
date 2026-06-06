#include "about.h"
#include "support/themeprovider.h"
#include "support/utils.h"
#include <redasm/redasm.h>

namespace {

constexpr int SCALE_LOGO = 128;
constexpr int SCALE_TEXT = 32;

void compile_config(QTextBrowser* txb) {
    const QString VERSION_CONTENT = R"(
<div><b>Qt Version:</b> %1</div>
<div><b>Core Version:</b> %2</div>
<div><b>RDAPI Level:</b> %3</div><br>
)";

    const QString SEARCH_PATHS_CONTENT = R"(
<div><b>Search Paths</b></div>
<div>%1</div>
)";

    txb->setLineWrapMode(QTextBrowser::NoWrap);
    txb->insertHtml(VERSION_CONTENT.arg(QT_VERSION_STR)
                        .arg(rd_build_version())
                        .arg(RD_API_LEVEL));

    if(!utils::search_paths.isEmpty()) {
        QString lines;

        for(const QString& sp : utils::search_paths)
            lines.append(QString{"<div>- %1</div>"}.arg(sp));

        txb->insertHtml(SEARCH_PATHS_CONTENT.arg(lines));
    }
    else {
        // clang-format off
        txb->insertHtml(SEARCH_PATHS_CONTENT.arg(QString{R"(
                <font color="%1">
                    <b>No Search paths set</b>
                </font>
        )"}.arg(theme_provider::color(RD_THEME_FAIL).name())));
        // clang-format on
    }
}

void compile_modules(QTextBrowser* txb) {
    const QString CONTENT = R"(
<table>
    <tr>
        <th valign="middle">Path</th>
        <th valign="middle">Version</th>
    </tr>
    %1
</table>
)";

    QString html;
    RDModuleSlice modules = rd_get_all_modules();

    const RDModule** it;
    rd_slice_each(it, modules) {
        const RDModule* m = *it;

        QString row = R"(
                <tr>
                    <td valign="middle">%1</td>
                    <td valign="middle">%2</td>
                </tr>
            )";

        html.append(row.arg(m->path).arg(m->version));
    }

    txb->setLineWrapMode(QTextBrowser::NoWrap);
    txb->setHtml(CONTENT.arg(html));
}

void compile_loaders(QTextBrowser* txb) {
    RDPluginSlice loaders = rd_get_all_loader_plugins();
    QString html;

    const RDPlugin** it;
    rd_slice_each(it, loaders) {
        const RDPlugin* p = *it;
        html.append(QString{"<div>- %1</div>"}.arg(p->loader->id));
    }

    txb->setLineWrapMode(QTextBrowser::NoWrap);
    txb->setHtml(html);
}

void compile_processors(QTextBrowser* txb) {
    const QString CONTENT = R"(
<table>
    <tr>
        <th valign="middle" width="50%">Name</th>
        <th valign="middle" width="50%">Id</th>
    </tr>
    %1
</table>
)";

    RDPluginSlice processors = rd_get_all_processor_plugins();
    QString html;

    const RDPlugin** it;
    rd_slice_each(it, processors) {
        const RDPlugin* p = *it;

        QString row = R"(
                <tr>
                    <td align="center" valign="middle">%1</td>
                    <td align="center" valign="middle">%2</td>
                </tr>
            )";

        html.append(row.arg(p->processor->name).arg(p->processor->id));
    }

    txb->setLineWrapMode(QTextBrowser::NoWrap);
    txb->setHtml(CONTENT.arg(html));
}

} // namespace

AboutDialog::AboutDialog(QWidget* parent): QDialog{parent}, m_ui{this} {
    this->setWindowTitle("About REDasm");

    m_ui.lbllogo->setPixmap(utils::get_logo().scaledToHeight(SCALE_LOGO));
    m_ui.lbltitle->setText("The OpenSource Disassembler");
    m_ui.lbltitle->setStyleSheet(QString{"font-size: %1px"}.arg(SCALE_TEXT));

    compile_config(m_ui.txbconfig);
    compile_modules(m_ui.txbmodules);
    compile_loaders(m_ui.txbloaders);
    compile_processors(m_ui.txbprocessors);
}
