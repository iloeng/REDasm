#include "qscrollarea64.h"
#include <QApplication>
#include <QWheelEvent>

QScrollArea64::QScrollArea64(QWidget* parent): QWidget{parent} {
    m_hbar = new QScrollBar64(Qt::Horizontal, this);
    m_vbar = new QScrollBar64(Qt::Vertical, this);

    QObject::connect(m_hbar, &QScrollBar64::valueChanged, this, [&](quint64 v) {
        auto dx = static_cast<qint64>(m_xoffset - v);
        m_xoffset = v;
        this->scrollContentsBy(dx, 0);
    });

    QObject::connect(m_hbar, &QScrollBar64::rangeChanged, this,
                     [&](quint64, quint64) { this->updateLayout(); });

    QObject::connect(m_vbar, &QScrollBar64::valueChanged, this, [&](quint64 v) {
        auto dy = static_cast<qint64>(m_yoffset - v);
        m_yoffset = v;
        this->scrollContentsBy(0, dy);
    });

    QObject::connect(m_vbar, &QScrollBar64::rangeChanged, this,
                     [&](quint64, quint64) { this->updateLayout(); });

    this->setFocusPolicy(Qt::StrongFocus);
    this->setViewport(nullptr);

    m_hbar->setRange(0, 0);
    m_vbar->setRange(0, 0);
}

QRect QScrollArea64::viewportRect() const {
    int v_extent =
        this->isVerticalScrollBarVisible() ? m_vbar->sizeHint().width() : 0;
    int h_extent =
        this->isHorizontalScrollBarVisible() ? m_hbar->sizeHint().height() : 0;

    int w = qMax(0, this->width() - v_extent);
    int h = qMax(0, this->height() - h_extent);
    return {0, 0, w, h};
}

void QScrollArea64::setViewport(QWidget* w) {
    if(!w) {
        w = new QWidget();
        w->setBackgroundRole(QPalette::Base);
        w->setAutoFillBackground(true);
    }

    if(m_viewport == w) return;

    QWidget* oldviewport = m_viewport;

    if(oldviewport) {
        m_viewport->removeEventFilter(this);
        m_viewport->setParent(nullptr);
    }

    m_viewport = w;

    if(m_viewport) {
        m_viewport->setParent(this);
        m_viewport->installEventFilter(this);
        m_viewport->setFocusProxy(this);
        m_viewport->show();

        m_hbar->raise();
        m_vbar->raise();
    }

    delete oldviewport;
    this->updateLayout();
}

void QScrollArea64::setHorizontalScrollBarVisible(bool visible) {
    if(visible == m_hbar->isVisible()) return;

    m_hbar->setVisible(visible);
    this->updateLayout();
}

void QScrollArea64::setVerticalScrollBarVisible(bool visible) {
    if(visible == m_vbar->isVisible()) return;

    m_vbar->setVisible(visible);
    this->updateLayout();
}

void QScrollArea64::setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy policy) {
    if(m_hpolicy == policy) return;

    m_hpolicy = policy;
    this->updateLayout();
}

void QScrollArea64::setVerticalScrollBarPolicy(Qt::ScrollBarPolicy policy) {
    if(m_vpolicy == policy) return;

    m_vpolicy = policy;
    this->updateLayout();
}

bool QScrollArea64::isHorizontalScrollBarVisible() const {
    if(m_hpolicy == Qt::ScrollBarAlwaysOff) return false;
    if(m_hpolicy == Qt::ScrollBarAlwaysOn) return true;
    return this->horizontalNeeded();
}

bool QScrollArea64::isVerticalScrollBarVisible() const {
    if(m_vpolicy == Qt::ScrollBarAlwaysOff) return false;
    if(m_vpolicy == Qt::ScrollBarAlwaysOn) return true;
    return this->verticalNeeded();
}

bool QScrollArea64::eventFilter(QObject* watched, QEvent* e) {
    return watched == m_viewport ? this->viewportEvent(e) : false;
}

void QScrollArea64::scrollContentsBy(qint64 dx, qint64 dy) {
    Q_UNUSED(dx);
    Q_UNUSED(dy);

    m_viewport->update();
}

bool QScrollArea64::viewportEvent(QEvent* e) {
    switch(e->type()) {
        case QEvent::Resize:
        case QEvent::Paint:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        case QEvent::MouseMove:
        case QEvent::ContextMenu:
        case QEvent::Wheel:
        case QEvent::Drop:
        case QEvent::DragEnter:
        case QEvent::DragMove:
        case QEvent::DragLeave: return QWidget::event(e);

        case QEvent::LayoutRequest:
        case QEvent::ScrollPrepare:
        case QEvent::Scroll: return this->event(e);

        default: break;
    }

    return false;
}

bool QScrollArea64::event(QEvent* e) {
    switch(e->type()) {
        case QEvent::Resize: {
            this->updateLayout();
            break;
        }

        case QEvent::Paint: {
            QWidget::paintEvent(static_cast<QPaintEvent*>(e));
            break;
        }

        default: return QWidget::event(e);
    }

    return true;
}

void QScrollArea64::resizeEvent(QResizeEvent* event) { Q_UNUSED(event); }

void QScrollArea64::wheelEvent(QWheelEvent* event) {
    QScrollBar64* target =
        (event->modifiers() & Qt::ShiftModifier) ? m_hbar : m_vbar;

    if(target && target->isVisible()) {
        QApplication::sendEvent(target, event);
    }
    else {
        QScrollBar64* fallback = (target == m_vbar) ? m_hbar : m_vbar;
        if(fallback && fallback->isVisible())
            QApplication::sendEvent(fallback, event);
        else
            event->ignore();
    }
}

void QScrollArea64::paintEvent(QPaintEvent* event) { Q_UNUSED(event); }

void QScrollArea64::updateLayout() {
    const bool H_VISIBLE = this->isHorizontalScrollBarVisible();
    const bool V_VISIBLE = this->isVerticalScrollBarVisible();

    int v_extent = V_VISIBLE ? m_vbar->sizeHint().width() : 0;
    int h_extent = H_VISIBLE ? m_hbar->sizeHint().height() : 0;
    QRect vp_rect = this->viewportRect();

    if(m_viewport) m_viewport->setGeometry(vp_rect);

    if(V_VISIBLE) {
        m_vbar->setGeometry(vp_rect.width(), 0, v_extent, vp_rect.height());
        m_vbar->show();
    }
    else
        m_vbar->hide();

    if(H_VISIBLE) {
        m_hbar->setGeometry(0, vp_rect.height(), vp_rect.width(), h_extent);
        m_hbar->show();
    }
    else
        m_hbar->hide();
}
