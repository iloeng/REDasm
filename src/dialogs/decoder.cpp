#include "decoder.h"
#include "support/themeprovider.h"
#include "support/utils.h"
#include <QFontDatabase>
#include <QHexView/model/buffer/qmemorybuffer.h>
#include <QTextBlock>
#include <redasm/redasm.h>

DecoderDialog::DecoderDialog(QWidget* parent): QDialog{parent}, m_ui{this} {
    utils::configure_hex_input(m_ui.leaddress);
    this->populate_processors();
    this->check_status();

    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_ui.dec_textedit->setFont(f);
    m_ui.enc_textedit->setFont(f);

    m_ui.leaddress->setText("0");

    // insert mode by default
    m_ui.dec_hexview->setCursorMode(QHexCursor::Mode::Insert);

    connect(m_ui.cbprocessors, &QComboBox::currentIndexChanged, this,
            &DecoderDialog::check_status);

    connect(
        m_ui.leaddress, &QLineEdit::textChanged, this, [&](const QString& s) {
            if(s.isEmpty()) {
                m_ui.dec_hexview->setBaseAddress(0);
                m_ui.enc_hexview->setBaseAddress(0);
            }
            else {
                m_ui.dec_hexview->setBaseAddress(s.toULongLong(nullptr, 16));
                m_ui.enc_hexview->setBaseAddress(s.toULongLong(nullptr, 16));
            }

            this->check_status();
        });

    connect(m_ui.dec_hexview, &QHexView::dataChanged, this,
            &DecoderDialog::check_status);

    connect(m_ui.enc_textedit, &QPlainTextEdit::textChanged, this,
            &DecoderDialog::check_status);

    connect(m_ui.pbclear, &QPushButton::clicked, this, [&]() {
        QHexDocument* olddocument = m_ui.dec_hexview->hexDocument();
        QHexDocument* newdocument = QHexDocument::fromMemory<QMemoryBuffer>(
            QByteArray{}, m_ui.dec_hexview);
        m_ui.dec_hexview->setDocument(newdocument);
        if(olddocument) olddocument->deleteLater();
        m_ui.leaddress->setText("0");
        this->check_status();
    });

    connect(m_ui.dec_buttonbox, &QDialogButtonBox::accepted, this,
            &DecoderDialog::do_decode);

    connect(m_ui.enc_buttonbox, &QDialogButtonBox::accepted, this,
            &DecoderDialog::do_encode);
}

void DecoderDialog::populate_processors() const {
    RDPluginSlice plugins = rd_get_all_processor_plugins();
    m_ui.cbprocessors->addItem(QString{}); // empty placeholder

    const RDPlugin** it;
    rd_slice_each(it, plugins) {
        m_ui.cbprocessors->addItem((*it)->processor->name,
                                   (*it)->processor->id);
    }
}

void DecoderDialog::check_status() { // NOLINT
    bool can_decode = !m_ui.leaddress->text().isEmpty() &&
                      m_ui.cbprocessors->currentIndex() > 0 &&
                      !m_ui.dec_hexview->hexDocument()->isEmpty();

    bool can_encode = !m_ui.leaddress->text().isEmpty() &&
                      m_ui.cbprocessors->currentIndex() > 0 &&
                      !m_ui.enc_textedit->document()->isEmpty();

    m_ui.dec_buttonbox->button(QDialogButtonBox::Ok)->setEnabled(can_decode);
    m_ui.enc_buttonbox->button(QDialogButtonBox::Ok)->setEnabled(can_encode);
}

void DecoderDialog::do_decode() { // NOLINT
    m_ui.dec_textedit->clear();

    const QHexDocument* doc = m_ui.dec_hexview->hexDocument();
    QByteArray ba = doc->read(0, static_cast<int>(doc->length()));
    const char* p = ba.data();
    auto len = static_cast<usize>(ba.length());

    QString straddr = m_ui.leaddress->text();
    auto addr = static_cast<RDAddress>(straddr.toULongLong(nullptr, 16));

    RDPluginSlice plugins = rd_get_all_processor_plugins();
    const RDProcessorPlugin* processor =
        rd_slice_at(plugins, m_ui.cbprocessors->currentIndex() - 1)->processor;

    RDDecodedInstruction dec;

    m_ui.dec_textedit->clear();
    QTextCursor cursor = m_ui.dec_textedit->textCursor();
    cursor.movePosition(QTextCursor::End);

    while(rd_decode_bytes(&p, &len, &addr, processor, &dec)) {
        if(!m_ui.dec_textedit->document()->isEmpty()) cursor.insertBlock();

        QString addrstr = QString::number(dec.instr.address, 16)
                              .rightJustified(sizeof(u32) * 2, '0');

        QTextCharFormat cf;
        cursor.insertText(addrstr, cf);
        cursor.insertText(" ", cf);
        cursor.insertText(QString::fromUtf8(dec.instr_text), cf);

        if(m_ui.dec_cbxdetails->isChecked()) {
            cursor.insertBlock();
            cf.setForeground(theme_provider::color(RD_THEME_COMMENT));

            // indent lines
            QStringList dump =
                QString::fromUtf8(rd_dump_instruction(&dec.instr)).split("\n");

            for(const QString& s : dump) {
                if(s != dump.front()) cursor.insertBlock();
                cursor.insertText("  ", cf);
                cursor.insertText(s, cf);
            }
        }
    }
}

void DecoderDialog::do_encode() { // NOLINT
    QHexDocument* hexdocument = m_ui.enc_hexview->hexDocument();
    hexdocument->clear();
    m_ui.enc_lblerror->clear();

    RDPluginSlice plugins = rd_get_all_processor_plugins();
    const RDProcessorPlugin* processor =
        rd_slice_at(plugins, m_ui.cbprocessors->currentIndex() - 1)->processor;

    QString straddr = m_ui.leaddress->text();
    auto addr = static_cast<RDAddress>(straddr.toULongLong(nullptr, 16));

    QTextDocument* doc = m_ui.enc_textedit->document();

    for(QTextBlock block = doc->begin(); block.isValid();
        block = block.next()) {
        QString line = block.text();
        if(line.isEmpty()) continue;

        const char* errmsg = nullptr;

        const RDScratchBuffer* buf = rd_encode_instruction(
            qUtf8Printable(line), addr, processor, &errmsg);

        if(!buf) {
            m_ui.enc_lblerror->setText(QString::fromUtf8(errmsg));
            break;
        }

        hexdocument->append(
            QByteArray{rd_scratch_data(buf),
                       static_cast<qsizetype>(rd_scratch_length(buf))});
    }
}
