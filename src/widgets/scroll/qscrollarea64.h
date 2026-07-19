#pragma once

// clang-format off
/* 
 * Based on:
 * - https://codebrowser.dev/qt6/qtbase/src/widgets/widgets/qabstractscrollarea.h.html
 * - https://codebrowser.dev/qt6/qtbase/src/widgets/widgets/qabstractscrollarea.cpp.html
 *
 * Linting is disabled because this widget follows Qt code style
 */
// clang-format on

#include "qscrollbar64.h"
#include <QWidget>

// NOLINTBEGIN
class QScrollArea64: public QWidget {
    Q_OBJECT

public:
    explicit QScrollArea64(QWidget* parent = nullptr);
    void setViewport(QWidget* w);
    void setHorizontalScrollBarVisible(bool visible);
    void setVerticalScrollBarVisible(bool visible);
    void setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy policy);
    void setVerticalScrollBarPolicy(Qt::ScrollBarPolicy policy);
    QRect viewportRect() const;
    QWidget* viewport() const { return m_viewport; }
    QScrollBar64* horizontalScrollBar() const { return m_hbar; }
    QScrollBar64* verticalScrollBar() const { return m_vbar; }
    Qt::ScrollBarPolicy horizontalScrollBarPolicy() const { return m_hpolicy; }
    Qt::ScrollBarPolicy verticalScrollBarPolicy() const { return m_vpolicy; }
    bool isHorizontalScrollBarVisible() const;
    bool isVerticalScrollBarVisible() const;
    bool eventFilter(QObject* watched, QEvent* e) override;

protected:
    virtual void scrollContentsBy(qint64 dx, qint64 dy);
    virtual bool viewportEvent(QEvent* e);
    bool event(QEvent* e) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void updateLayout();

    bool horizontalNeeded() const {
        return m_hbar->maximum() > m_hbar->minimum();
    }

    bool verticalNeeded() const {
        return m_vbar->maximum() > m_vbar->minimum();
    }

private:
    QScrollBar64* m_hbar;
    QScrollBar64* m_vbar;
    Qt::ScrollBarPolicy m_hpolicy{Qt::ScrollBarAsNeeded};
    Qt::ScrollBarPolicy m_vpolicy{Qt::ScrollBarAsNeeded};
    QWidget* m_viewport{nullptr};
    qint64 m_xoffset{0};
    qint64 m_yoffset{0};
};
// NOLINTEND
