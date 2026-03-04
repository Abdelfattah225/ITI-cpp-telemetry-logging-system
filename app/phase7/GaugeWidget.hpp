#pragma once

#include <QWidget>
#include <QString>
#include <QColor>

/**
 * @file GaugeWidget.hpp
 * @brief Custom circular speedometer-style gauge widget for Qt6
 *
 * Draws a car-instrument-cluster style gauge with:
 * - Glowing colored arc (0 to maxValue)
 * - Animated needle
 * - Large bold value in center
 * - Color zones: green → yellow → red
 */
class GaugeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GaugeWidget(QWidget* parent = nullptr);

    void setLabel(const QString& label);
    void setUnit(const QString& unit);
    void setRange(double min, double max);
    void setValue(double value);
    void setColor(const QColor& color);

    // Color zone thresholds (as % of max)
    void setWarnThreshold(double warnPct);   // default 60%
    void setCritThreshold(double critPct);   // default 80%

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    void updateValue(double value);

signals:
    void valueChanged(double value);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_label;
    QString m_unit;
    double  m_min        = 0.0;
    double  m_max        = 100.0;
    double  m_value      = 0.0;
    double  m_warnPct    = 0.60;
    double  m_critPct    = 0.80;
    QColor  m_baseColor;       // Primary glow color

    // Animation
    double  m_displayValue = 0.0; // smoothed animated value

    QColor colorForValue() const;
    void   drawBackground(QPainter& p, const QRectF& rect);
    void   drawArc(QPainter& p, const QRectF& rect);
    void   drawNeedle(QPainter& p, const QRectF& rect);
    void   drawCenter(QPainter& p, const QRectF& rect);
    void   drawTicks(QPainter& p, const QRectF& rect);
};
