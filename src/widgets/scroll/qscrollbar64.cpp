#include "qscrollbar64.h"
#include <QApplication>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStylePainter>
#include <QWheelEvent>

QScrollBar64::QScrollBar64(Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent) {
    this->initFrom(orientation);
}

QScrollBar64::QScrollBar64(QWidget* parent): QWidget(parent) {
    this->initFrom(Qt::Vertical);
}

QScrollBar64::~QScrollBar64() = default;

void QScrollBar64::initFrom(Qt::Orientation orientation) {
    m_orientation = orientation;

    this->setAttribute(Qt::WA_Hover, true);
    this->setAttribute(Qt::WA_OpaquePaintEvent, true);
    this->setFocusPolicy(Qt::StrongFocus);

    if(orientation == Qt::Horizontal)
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    else
        this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

quint64 QScrollBar64::clampToRange(quint64 v) const {
    if(m_maximum <= m_minimum) return m_minimum;

    return qMin(m_maximum, qMax(m_minimum, v));
}

quint64 QScrollBar64::saturatingSub(quint64 v, quint64 step, quint64 floor) {
    if(v <= floor) return floor;

    quint64 avail = v - floor; // safe: v > floor here
    return (step >= avail) ? floor : v - step;
}

quint64 QScrollBar64::saturatingAdd(quint64 v, quint64 step, quint64 ceiling) {
    if(v >= ceiling) return ceiling;

    quint64 avail = ceiling - v; // safe: ceiling > v here
    return (step >= avail) ? ceiling : v + step;
}

void QScrollBar64::setMinimum(quint64 min) { this->setRange(min, m_maximum); }
void QScrollBar64::setMaximum(quint64 max) { this->setRange(m_minimum, max); }

void QScrollBar64::setRange(quint64 min, quint64 max) {
    max = qMax(max, min);

    if(min == m_minimum && max == m_maximum) return;

    m_minimum = min;
    m_maximum = max;

    Q_EMIT rangeChanged(m_minimum, m_maximum);

    quint64 clamped = clampToRange(m_value);

    if(clamped != m_value) {
        m_value = clamped;
        Q_EMIT valueChanged(m_value);
    }

    this->update();
}

void QScrollBar64::setPageStep(quint64 step) {
    if(step == m_pagestep) return;

    m_pagestep = step;
    this->update();
}

void QScrollBar64::setSingleStep(quint64 step) { m_singlestep = step; }

void QScrollBar64::setValue(quint64 value) {
    this->setValueInternal(value, true, false);
}

void QScrollBar64::setValueInternal(quint64 v, bool emit_signals,
                                    bool is_slider_move) {
    v = this->clampToRange(v);

    if(v == m_value) {
        if(emit_signals && is_slider_move) Q_EMIT sliderMoved(m_value);
        return;
    }

    m_value = v;
    this->update();

    if(emit_signals) {
        if(is_slider_move) Q_EMIT sliderMoved(m_value);
        Q_EMIT valueChanged(m_value);
    }
}

void QScrollBar64::setOrientation(Qt::Orientation orientation) {
    if(m_orientation == orientation) return;

    m_orientation = orientation;

    if(orientation == Qt::Horizontal)
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    else
        this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    this->updateGeometry();
    this->update();
}

void QScrollBar64::setInvertedAppearance(bool inverted) {
    if(m_invertedappearance == inverted) return;

    m_invertedappearance = inverted;
    this->update();
}

// ---------------------------------------------------------------------
// 64-bit <-> virtual-int mapping
//
// The style only understands 'int' ranges, so we hand it a fixed virtual
// span [0, kVirtualMax] to compute handle/groove pixel geometry, then map
// pixel positions back to our real quint64 value ourselves.
// The multiply is done in 128 bits (where the compiler supports it) so the
// mapping stays exact even near the top of a 2^64-sized range.
// ---------------------------------------------------------------------

quint64 QScrollBar64::virtualToValue(int virtual_pos) const {
    if(m_maximum <= m_minimum) return m_minimum;
    if(virtual_pos <= 0) return m_minimum;
    if(virtual_pos >= kVirtualMax) return m_maximum;

    quint64 range = m_maximum - m_minimum;

#if defined(__SIZEOF_INT128__)
    __uint128_t num =
        static_cast<__uint128_t>(static_cast<quint64>(virtual_pos)) *
        static_cast<__uint128_t>(range);
    auto offset = static_cast<quint64>(num / static_cast<quint64>(kVirtualMax));
#else
    long double ratio = static_cast<long double>(virtual_pos) /
                        static_cast<long double>(kVirtualMax);
    auto offset = static_cast<quint64>(ratio * static_cast<long double>(range));
#endif

    return m_minimum + offset;
}

int QScrollBar64::valueToVirtual(quint64 v) const {
    if(m_maximum <= m_minimum) return 0;
    v = clampToRange(v);
    quint64 range = m_maximum - m_minimum;
    quint64 pos = v - m_minimum;

#if defined(__SIZEOF_INT128__)
    __uint128_t num =
        static_cast<__uint128_t>(pos) *
        static_cast<__uint128_t>(static_cast<quint64>(kVirtualMax));
    auto virt = static_cast<quint64>(num / range);
#else
    long double ratio =
        static_cast<long double>(pos) / static_cast<long double>(range);
    auto virt =
        static_cast<quint64>(ratio * static_cast<long double>(kVirtualMax));
#endif

    if(virt > static_cast<quint64>(kVirtualMax)) virt = kVirtualMax;
    return static_cast<int>(virt);
}

QStyleOptionSlider QScrollBar64::styleOption() const {
    QStyleOptionSlider opt;
    opt.initFrom(this);
    opt.subControls = QStyle::SC_All;
    opt.activeSubControls = (m_pressedcontrol != QStyle::SC_None)
                                ? m_pressedcontrol
                                : m_hovelcontrol;
    opt.orientation = m_orientation;
    opt.minimum = 0;
    opt.maximum = (m_maximum > m_minimum) ? kVirtualMax : 0;
    opt.sliderPosition = this->valueToVirtual(m_value);
    opt.sliderValue = opt.sliderPosition;

    opt.singleStep = qMax(1, this->valueToVirtual(this->saturatingAdd(
                                 m_minimum, m_singlestep, m_maximum)));
    opt.pageStep = qMax(1, this->valueToVirtual(this->saturatingAdd(
                               m_minimum, m_pagestep, m_maximum)));
    opt.upsideDown = m_invertedappearance;

    if(m_orientation == Qt::Horizontal) opt.state |= QStyle::State_Horizontal;

    if(this->isEnabled())
        opt.state |= QStyle::State_Enabled;
    else
        opt.state &= ~QStyle::State_Enabled;

    if(m_sliderdown) opt.state |= QStyle::State_Sunken;

    if(this->hasFocus()) opt.state |= QStyle::State_HasFocus;

    opt.rect = this->rect();
    return opt;
}

void QScrollBar64::paintEvent(QPaintEvent*) {
    QStylePainter painter(this);

    QStyleOptionSlider opt = this->styleOption();
    painter.drawComplexControl(QStyle::CC_ScrollBar, opt);
}

void QScrollBar64::mousePressEvent(QMouseEvent* event) {
    if(m_maximum <= m_minimum || event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    QStyleOptionSlider opt = this->styleOption();
    QStyle::SubControl sc = this->style()->hitTestComplexControl(
        QStyle::CC_ScrollBar, &opt, event->pos(), this);

    if(sc == QStyle::SC_None) {
        event->ignore();
        return;
    }

    m_pressedcontrol = sc;
    this->update();

    switch(sc) {
        case QStyle::SC_ScrollBarSlider: {
            m_sliderdown = true;

            QRect sr = this->style()->subControlRect(
                QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this);

            int click_pos = (m_orientation == Qt::Horizontal)
                                ? event->pos().x()
                                : event->pos().y();
            int slider_start =
                (m_orientation == Qt::Horizontal) ? sr.left() : sr.top();

            m_pressedpixeloffset = click_pos - slider_start;
            Q_EMIT sliderPressed();
            break;
        }

        case QStyle::SC_ScrollBarAddLine:
            this->startRepeatAction(QAbstractSlider::SliderSingleStepAdd);
            break;

        case QStyle::SC_ScrollBarSubLine:
            this->startRepeatAction(QAbstractSlider::SliderSingleStepSub);
            break;

        case QStyle::SC_ScrollBarAddPage:
            this->startRepeatAction(QAbstractSlider::SliderPageStepAdd);
            break;

        case QStyle::SC_ScrollBarSubPage:
            this->startRepeatAction(QAbstractSlider::SliderPageStepSub);
            break;

        default: break;
    }
}

void QScrollBar64::mouseMoveEvent(QMouseEvent* event) {
    QStyleOptionSlider opt = this->styleOption();

    // Keep hover highlight in sync even when not dragging.
    QStyle::SubControl hover = this->style()->hitTestComplexControl(
        QStyle::CC_ScrollBar, &opt, event->pos(), this);

    if(hover != m_hovelcontrol) {
        m_hovelcontrol = hover;
        this->update();
    }

    if(!m_sliderdown || m_pressedcontrol != QStyle::SC_ScrollBarSlider) {
        event->ignore();
        return;
    }

    QRect gr = this->style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                             QStyle::SC_ScrollBarGroove, this);
    QRect sr = this->style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                             QStyle::SC_ScrollBarSlider, this);

    int pos =
        (m_orientation == Qt::Horizontal) ? event->pos().x() : event->pos().y();
    int groove_start = (m_orientation == Qt::Horizontal) ? gr.left() : gr.top();
    int groove_len =
        (m_orientation == Qt::Horizontal) ? gr.width() : gr.height();
    int slider_len =
        (m_orientation == Qt::Horizontal) ? sr.width() : sr.height();
    int available = qMax(0, groove_len - slider_len);

    int new_slider_start = pos - m_pressedpixeloffset - groove_start;
    new_slider_start = qMin(available, qMax(0, new_slider_start));

    int virtual_pos = QStyle::sliderValueFromPosition(
        0, kVirtualMax, new_slider_start, available, opt.upsideDown);

    quint64 new_value = this->virtualToValue(virtual_pos);
    this->setValueInternal(new_value, true, true);
}

void QScrollBar64::mouseReleaseEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    this->stopRepeatAction();

    if(m_sliderdown) {
        m_sliderdown = false;
        Q_EMIT sliderReleased();
    }

    if(m_pressedcontrol != QStyle::SC_None) {
        m_pressedcontrol = QStyle::SC_None;
        this->update();
    }
}

void QScrollBar64::wheelEvent(QWheelEvent* event) {
    int delta = (event->angleDelta().y() != 0) ? event->angleDelta().y()
                                               : event->angleDelta().x();
    if(delta == 0) {
        event->ignore();
        return;
    }

    // Standard notch size is 120 units; scroll by singleStep per notch,
    // scaled by the platform's configured wheel scroll-lines.
    int lines = qMax(1, QApplication::wheelScrollLines());
    int notches = delta / 120;
    if(notches == 0) notches = (delta > 0) ? 1 : -1;

    quint64 magnitude = static_cast<quint64>(qAbs(notches)) * m_singlestep *
                        static_cast<quint64>(lines);

    // Wheel-up (positive delta, notches > 0) traditionally scrolls content
    // up, i.e. decreases the value; wheel-down increases it.
    quint64 new_value =
        (notches > 0) ? this->saturatingSub(m_value, magnitude, m_minimum)
                      : this->saturatingAdd(m_value, magnitude, m_maximum);

    this->setValue(new_value);
    event->accept();
}

void QScrollBar64::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    this->update();
}

void QScrollBar64::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);

    switch(event->type()) {
        case QEvent::EnabledChange:
        case QEvent::StyleChange:
        case QEvent::PaletteChange: this->update(); break;
        default: break;
    }
}

bool QScrollBar64::event(QEvent* e) {
    switch(e->type()) {
        case QEvent::HoverEnter:
        case QEvent::HoverMove: {
            auto* he = static_cast<QHoverEvent*>(e);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QPoint p = he->position().toPoint();
#else
            QPoint p = he->pos();
#endif

            QStyleOptionSlider opt = this->styleOption();
            QStyle::SubControl sc = this->style()->hitTestComplexControl(
                QStyle::CC_ScrollBar, &opt, p, this);

            if(sc != m_hovelcontrol) {
                m_hovelcontrol = sc;
                this->update();
            }

            break;
        }

        case QEvent::HoverLeave:
            if(m_hovelcontrol != QStyle::SC_None) {
                m_hovelcontrol = QStyle::SC_None;
                this->update();
            }

            break;

        default: break;
    }

    return QWidget::event(e);
}

void QScrollBar64::triggerAction(QAbstractSlider::SliderAction action) {
    quint64 new_value = m_value;

    switch(action) {
        case QAbstractSlider::SliderSingleStepAdd:
            new_value = this->saturatingAdd(m_value, m_singlestep, m_maximum);
            break;

        case QAbstractSlider::SliderSingleStepSub:
            new_value = this->saturatingSub(m_value, m_singlestep, m_minimum);
            break;

        case QAbstractSlider::SliderPageStepAdd:
            new_value = this->saturatingAdd(m_value, m_pagestep, m_maximum);
            break;

        case QAbstractSlider::SliderPageStepSub:
            new_value = this->saturatingSub(m_value, m_pagestep, m_minimum);
            break;

        case QAbstractSlider::SliderToMinimum: new_value = m_minimum; break;
        case QAbstractSlider::SliderToMaximum: new_value = m_maximum; break;
        default: break;
    }

    Q_EMIT actionTriggered(static_cast<int>(action));
    this->setValueInternal(new_value, true, false);
}

void QScrollBar64::startRepeatAction(QAbstractSlider::SliderAction action) {
    m_repeatact = action;
    m_repeattimercount = 0;
    this->doRepeatAction(); // fire once immediately

    // then wait, like QScrollBar's initial delay
    m_repeattimer.start(350, this);
}

void QScrollBar64::stopRepeatAction() {
    if(m_repeattimer.isActive()) m_repeattimer.stop();

    m_repeatact = QAbstractSlider::SliderNoAction;
}

void QScrollBar64::doRepeatAction() {
    this->triggerAction(m_repeatact);
    m_repeattimercount++;

    if(m_repeattimercount == 1)
        m_repeattimer.start(75, this); // faster repeat after the initial delay
}

void QScrollBar64::timerEvent(QTimerEvent* event) {
    if(event->timerId() == m_repeattimer.timerId())
        this->doRepeatAction();
    else
        QWidget::timerEvent(event);
}

QSize QScrollBar64::sizeHint() const {
    this->ensurePolished();

    QStyleOptionSlider opt = this->styleOption();
    int extent =
        this->style()->pixelMetric(QStyle::PM_ScrollBarExtent, &opt, this);
    return (m_orientation == Qt::Horizontal) ? QSize(extent * 2, extent)
                                             : QSize(extent, extent * 2);
}

QSize QScrollBar64::minimumSizeHint() const {
    QStyleOptionSlider opt = this->styleOption();
    int extent =
        this->style()->pixelMetric(QStyle::PM_ScrollBarExtent, &opt, this);
    return (m_orientation == Qt::Horizontal) ? QSize(extent * 2, extent)
                                             : QSize(extent, extent * 2);
}
