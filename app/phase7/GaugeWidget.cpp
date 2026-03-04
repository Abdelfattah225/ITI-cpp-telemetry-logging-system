#include "GaugeWidget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QConicalGradient>
#include <QRadialGradient>
#include <QFontDatabase>
#include <QtMath>
#include <QTimer>

// Gauge arc goes from 220° (bottom-left) to -40° (bottom-right) → 240° sweep
static constexpr double START_ANGLE_DEG = 220.0;  // degrees (Qt: CCW from 3-o'clock)
static constexpr double SWEEP_ANGLE_DEG = 240.0;

GaugeWidget::GaugeWidget(QWidget* parent)
    : QWidget(parent)
    , m_baseColor(Qt::cyan)
{
    setMinimumSize(200, 200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Smooth animation timer — update display value toward target
    QTimer* anim = new QTimer(this);
    connect(anim, &QTimer::timeout, this, [this]() {
        double diff = m_value - m_displayValue;
        if (std::abs(diff) < 0.1) {
            m_displayValue = m_value;
        } else {
            m_displayValue += diff * 0.15; // ease-in
        }
        update();
    });
    anim->start(16); // ~60fps
}

void GaugeWidget::setLabel(const QString& label) { m_label = label; update(); }
void GaugeWidget::setUnit(const QString& unit)   { m_unit  = unit;  update(); }
void GaugeWidget::setRange(double min, double max) { m_min = min; m_max = max; update(); }
void GaugeWidget::setColor(const QColor& color)  { m_baseColor = color; update(); }
void GaugeWidget::setWarnThreshold(double p)     { m_warnPct = p; update(); }
void GaugeWidget::setCritThreshold(double p)     { m_critPct = p; update(); }

void GaugeWidget::setValue(double value)
{
    m_value = qBound(m_min, value, m_max);
    emit valueChanged(m_value);
}

void GaugeWidget::updateValue(double value) { setValue(value); }

QSize GaugeWidget::sizeHint() const        { return {280, 280}; }
QSize GaugeWidget::minimumSizeHint() const { return {180, 180}; }

QColor GaugeWidget::colorForValue() const
{
    double pct = (m_max > m_min) ? (m_displayValue - m_min) / (m_max - m_min) : 0.0;
    if (pct >= m_critPct) return QColor(255, 60, 60);    // red
    if (pct >= m_warnPct) return QColor(255, 200, 0);    // amber
    return m_baseColor;
}

void GaugeWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int  side = qMin(width(), height());
    const qreal pad = side * 0.05;
    QRectF rect(pad, pad, side - 2*pad, side - 2*pad);
    // Center the rect
    rect.moveCenter(QPointF(width() / 2.0, height() / 2.0));

    drawBackground(p, rect);
    drawTicks(p, rect);
    drawArc(p, rect);
    drawNeedle(p, rect);
    drawCenter(p, rect);
}

void GaugeWidget::drawBackground(QPainter& p, const QRectF& rect)
{
    // Outer dark ring
    p.setPen(Qt::NoPen);
    QRadialGradient bg(rect.center(), rect.width() / 2.0);
    bg.setColorAt(0.0, QColor(30, 32, 45));
    bg.setColorAt(0.75, QColor(18, 20, 30));
    bg.setColorAt(1.0, QColor(10, 10, 15));
    p.setBrush(bg);
    p.drawEllipse(rect);

    // Subtle outer rim
    p.setPen(QPen(QColor(60, 65, 90), 2));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(rect.adjusted(1, 1, -1, -1));
}

void GaugeWidget::drawTicks(QPainter& p, const QRectF& rect)
{
    const double cx = rect.center().x();
    const double cy = rect.center().y();
    const double r  = rect.width() / 2.0;

    p.setPen(QPen(QColor(80, 85, 110), 1.5));

    int numTicks = 10;
    for (int i = 0; i <= numTicks; ++i) {
        double angleDeg = START_ANGLE_DEG - (SWEEP_ANGLE_DEG * i / numTicks);
        double rad = qDegreesToRadians(angleDeg);
        double innerR = r * 0.82;
        double outerR = r * 0.92;

        p.drawLine(
            QPointF(cx + innerR * qCos(rad), cy - innerR * qSin(rad)),
            QPointF(cx + outerR * qCos(rad), cy - outerR * qSin(rad))
        );
    }
}

void GaugeWidget::drawArc(QPainter& p, const QRectF& rect)
{
    const double pct = (m_max > m_min) ? (m_displayValue - m_min) / (m_max - m_min) : 0.0;
    const double valueSweep = SWEEP_ANGLE_DEG * pct;

    const QRectF arcRect = rect.adjusted(
        rect.width() * 0.08, rect.height() * 0.08,
        -rect.width() * 0.08, -rect.height() * 0.08
    );

    QColor arcColor = colorForValue();

    // Background arc (track)
    QPen trackPen(QColor(40, 44, 60), rect.width() * 0.055);
    trackPen.setCapStyle(Qt::RoundCap);
    p.setPen(trackPen);
    p.setBrush(Qt::NoBrush);
    p.drawArc(arcRect,
        static_cast<int>(START_ANGLE_DEG * 16),
        static_cast<int>(-SWEEP_ANGLE_DEG * 16));

    if (pct > 0.001) {
        // Glow effect — draw wider semi-transparent arc first
        QPen glowPen(QColor(arcColor.red(), arcColor.green(), arcColor.blue(), 60),
                     rect.width() * 0.09);
        glowPen.setCapStyle(Qt::RoundCap);
        p.setPen(glowPen);
        p.drawArc(arcRect,
            static_cast<int>(START_ANGLE_DEG * 16),
            static_cast<int>(-valueSweep * 16));

        // Main arc
        QPen arcPen(arcColor, rect.width() * 0.055);
        arcPen.setCapStyle(Qt::RoundCap);
        p.setPen(arcPen);
        p.drawArc(arcRect,
            static_cast<int>(START_ANGLE_DEG * 16),
            static_cast<int>(-valueSweep * 16));
    }
}

void GaugeWidget::drawNeedle(QPainter& p, const QRectF& rect)
{
    const double pct = (m_max > m_min) ? (m_displayValue - m_min) / (m_max - m_min) : 0.0;
    const double angleDeg = START_ANGLE_DEG - SWEEP_ANGLE_DEG * pct;
    const double rad = qDegreesToRadians(angleDeg);

    const double cx = rect.center().x();
    const double cy = rect.center().y();
    const double r  = rect.width() / 2.0;

    QPointF tip(cx + r * 0.60 * qCos(rad), cy - r * 0.60 * qSin(rad));
    QPointF base(cx - r * 0.12 * qCos(rad), cy + r * 0.12 * qSin(rad));

    QPen needlePen(QColor(230, 235, 255), r * 0.025);
    needlePen.setCapStyle(Qt::RoundCap);
    p.setPen(needlePen);
    p.drawLine(base, tip);

    // Center pivot dot
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(200, 205, 230));
    p.drawEllipse(rect.center(), r * 0.06, r * 0.06);
}

void GaugeWidget::drawCenter(QPainter& p, const QRectF& rect)
{
    QColor arcColor = colorForValue();
    const double r = rect.width() / 2.0;

    // Value text
    QString valStr = QString::number(static_cast<int>(m_displayValue));
    QFont valFont("Segoe UI", static_cast<int>(r * 0.38), QFont::Bold);
    valFont.setStyleHint(QFont::SansSerif);
    p.setFont(valFont);
    p.setPen(arcColor);

    QRectF valRect = rect.adjusted(rect.width()*0.1, rect.height()*0.3,
                                   -rect.width()*0.1, -rect.height()*0.25);
    p.drawText(valRect, Qt::AlignCenter, valStr);

    // Unit text below value
    QFont unitFont("Segoe UI", static_cast<int>(r * 0.16));
    p.setFont(unitFont);
    p.setPen(QColor(160, 165, 190));
    QRectF unitRect = rect.adjusted(rect.width()*0.1, rect.height()*0.60,
                                    -rect.width()*0.1, -rect.height()*0.08);
    p.drawText(unitRect, Qt::AlignCenter | Qt::AlignTop, m_unit);

    // Label at bottom
    QFont lblFont("Segoe UI", static_cast<int>(r * 0.14));
    p.setFont(lblFont);
    p.setPen(QColor(120, 125, 150));
    QRectF lblRect(rect.left(), rect.bottom() + 6, rect.width(), r * 0.35);
    p.drawText(lblRect, Qt::AlignCenter, m_label);
}
