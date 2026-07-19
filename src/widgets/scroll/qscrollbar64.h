#pragma once

#include <QAbstractSlider> // for QAbstractSlider::SliderAction enum reuse
#include <QBasicTimer>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QWidget>

// NOLINTBEGIN
class QScrollBar64: public QWidget {
    Q_OBJECT
    Q_PROPERTY(quint64 minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(quint64 maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(quint64 pageStep READ pageStep WRITE setPageStep)
    Q_PROPERTY(quint64 singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(
        quint64 value READ value WRITE setValue NOTIFY valueChanged USER true)
    Q_PROPERTY(
        Qt::Orientation orientation READ orientation WRITE setOrientation)
    Q_PROPERTY(bool invertedAppearance READ invertedAppearance WRITE
                   setInvertedAppearance)
    Q_PROPERTY(
        bool invertedControls READ invertedControls WRITE setInvertedControls)

public:
    explicit QScrollBar64(Qt::Orientation orientation,
                          QWidget* parent = nullptr);
    explicit QScrollBar64(QWidget* parent = nullptr);
    ~QScrollBar64() override;

    quint64 minimum() const { return m_minimum; }
    quint64 maximum() const { return m_maximum; }
    void setMinimum(quint64 min);
    void setMaximum(quint64 max);
    void setRange(quint64 min, quint64 max);

    quint64 pageStep() const { return m_pagestep; }
    void setPageStep(quint64 step);

    quint64 singleStep() const { return m_singlestep; }
    void setSingleStep(quint64 step);

    quint64 value() const { return m_value; }

    Qt::Orientation orientation() const { return m_orientation; }
    void setOrientation(Qt::Orientation orientation);

    bool invertedAppearance() const { return m_invertedappearance; }
    void setInvertedAppearance(bool inverted);

    bool invertedControls() const { return m_invertedcontrols; }
    void setInvertedControls(bool inverted) { m_invertedcontrols = inverted; }

    bool isSliderDown() const { return m_sliderdown; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public Q_SLOTS:
    void setValue(quint64 value);
    void triggerAction(QAbstractSlider::SliderAction action);

Q_SIGNALS:
    void valueChanged(quint64 value);
    void rangeChanged(quint64 min, quint64 max);
    void sliderMoved(quint64 value);
    void sliderPressed();
    void sliderReleased();
    void actionTriggered(int action);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void timerEvent(QTimerEvent* event) override;
    void changeEvent(QEvent* event) override;
    bool event(QEvent* event) override;

private:
    void initFrom(Qt::Orientation orientation);
    QStyleOptionSlider styleOption() const;
    quint64 clampToRange(quint64 v) const;
    static quint64 saturatingSub(quint64 v, quint64 step, quint64 floor);
    static quint64 saturatingAdd(quint64 v, quint64 step, quint64 ceiling);
    quint64 virtualToValue(int virtualPos) const;
    int valueToVirtual(quint64 v) const;
    void setValueInternal(quint64 v, bool emitSignals, bool isSliderMove);
    void startRepeatAction(QAbstractSlider::SliderAction action);
    void stopRepeatAction();
    void doRepeatAction();

    static constexpr int kVirtualMax =
        1000 * 1000 * 1000; // ~1e9 virtual steps for style geometry math

    quint64 m_minimum{0};
    quint64 m_maximum{99};
    quint64 m_value{0};
    quint64 m_singlestep{1};
    quint64 m_pagestep{10};
    Qt::Orientation m_orientation{Qt::Vertical};
    bool m_invertedappearance{false};
    bool m_invertedcontrols{false};

    bool m_sliderdown{false};
    QStyle::SubControl m_pressedcontrol{QStyle::SC_None};
    QStyle::SubControl m_hovelcontrol{QStyle::SC_None};
    int m_pressedpixeloffset{0};

    QBasicTimer m_repeattimer;
    QAbstractSlider::SliderAction m_repeatact{QAbstractSlider::SliderNoAction};
    int m_repeattimercount{0};
};
// NOLINTEND
