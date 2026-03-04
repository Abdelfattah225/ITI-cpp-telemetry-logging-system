#include "DashboardWindow.hpp"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QFrame>
#include <QFont>
#include <QStatusBar>
#include <QPalette>
#include <QStyle>

DashboardWindow::DashboardWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("RPi System Monitor — Phase 7");
    setMinimumSize(900, 520);
    resize(1000, 600);

    applyStyleSheet();
    setupUi();
}

void DashboardWindow::applyStyleSheet()
{
    // Global dark theme
    qApp->setStyle("Fusion");

    QPalette dark;
    dark.setColor(QPalette::Window,          QColor(10,  12,  20));
    dark.setColor(QPalette::WindowText,      QColor(220, 225, 240));
    dark.setColor(QPalette::Base,            QColor(14,  16,  26));
    dark.setColor(QPalette::AlternateBase,   QColor(18,  20,  32));
    dark.setColor(QPalette::ToolTipBase,     QColor(30,  35,  55));
    dark.setColor(QPalette::ToolTipText,     Qt::white);
    dark.setColor(QPalette::Text,            QColor(210, 215, 235));
    dark.setColor(QPalette::Button,          QColor(25,  28,  42));
    dark.setColor(QPalette::ButtonText,      QColor(210, 215, 235));
    dark.setColor(QPalette::Highlight,       QColor(0, 180, 220));
    dark.setColor(QPalette::HighlightedText, Qt::black);
    qApp->setPalette(dark);

    setStyleSheet(R"(
        QMainWindow { background-color: #0a0c14; }
        QStatusBar  { background-color: #0d1020; color: #6a6f90;
                      border-top: 1px solid #1e2238; font-size: 11px; }
        QLabel#titleLabel { font-size: 22px; font-weight: bold; color: #dde2f0; }
        QLabel#hostLabel  { font-size: 11px; color: #555a7a; }
        QLabel#connBadge[connected="true"]  { background-color: #0d2b14; color: #30e060;
            border: 1px solid #1a6030; border-radius: 10px;
            padding: 3px 10px; font-size: 12px; font-weight: bold; }
        QLabel#connBadge[connected="false"] { background-color: #2b0d0d; color: #e03030;
            border: 1px solid #601a1a; border-radius: 10px;
            padding: 3px 10px; font-size: 12px; font-weight: bold; }
        QFrame#separator { background-color: #1e2238; }
    )");
}

void DashboardWindow::setupUi()
{
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout* vbox = new QVBoxLayout(central);
    vbox->setContentsMargins(24, 18, 24, 12);
    vbox->setSpacing(16);

    setupHeader();
    vbox->addWidget(m_titleLabel->parentWidget());   // header container

    // Horizontal separator
    QFrame* sep = new QFrame();
    sep->setObjectName("separator");
    sep->setFixedHeight(1);
    vbox->addWidget(sep);

    setupGauges();
    // Gauge container
    QWidget* gaugeContainer = new QWidget();
    QHBoxLayout* gaugeRow = new QHBoxLayout(gaugeContainer);
    gaugeRow->setContentsMargins(0, 0, 0, 0);
    gaugeRow->setSpacing(40);
    gaugeRow->addWidget(m_cpuLoadGauge);
    gaugeRow->addWidget(m_cpuTempGauge);
    gaugeRow->addWidget(m_ramGauge);
    vbox->addWidget(gaugeContainer, 1);

    setupStatusBar();
}

void DashboardWindow::setupHeader()
{
    QWidget* header = new QWidget();
    QHBoxLayout* hbox = new QHBoxLayout(header);
    hbox->setContentsMargins(0, 0, 0, 0);

    // Left side: title + host
    QVBoxLayout* titleCol = new QVBoxLayout();
    m_titleLabel = new QLabel("🍓  RPi System Monitor");
    m_titleLabel->setObjectName("titleLabel");
    m_hostLabel  = new QLabel("vSOME/IP | Connecting...");
    m_hostLabel->setObjectName("hostLabel");
    titleCol->addWidget(m_titleLabel);
    titleCol->addWidget(m_hostLabel);

    // Right side: connection badge
    m_connBadge = new QLabel("● CONNECTING");
    m_connBadge->setObjectName("connBadge");
    m_connBadge->setProperty("connected", false);
    m_connBadge->setAlignment(Qt::AlignCenter);
    m_connBadge->setFixedHeight(28);

    hbox->addLayout(titleCol);
    hbox->addStretch();
    hbox->addWidget(m_connBadge);
}

void DashboardWindow::setupGauges()
{
    // CPU Load
    m_cpuLoadGauge = new GaugeWidget();
    m_cpuLoadGauge->setLabel("CPU Load");
    m_cpuLoadGauge->setUnit("%");
    m_cpuLoadGauge->setRange(0, 100);
    m_cpuLoadGauge->setColor(QColor(0, 210, 200));    // cyan
    m_cpuLoadGauge->setWarnThreshold(0.60);
    m_cpuLoadGauge->setCritThreshold(0.85);
    m_cpuLoadGauge->setValue(0);

    // CPU Temp
    m_cpuTempGauge = new GaugeWidget();
    m_cpuTempGauge->setLabel("CPU Temp");
    m_cpuTempGauge->setUnit("°C");
    m_cpuTempGauge->setRange(0, 100);
    m_cpuTempGauge->setColor(QColor(255, 140, 0));    // orange
    m_cpuTempGauge->setWarnThreshold(0.55);
    m_cpuTempGauge->setCritThreshold(0.75);
    m_cpuTempGauge->setValue(0);

    // RAM Usage
    m_ramGauge = new GaugeWidget();
    m_ramGauge->setLabel("RAM Usage");
    m_ramGauge->setUnit("%");
    m_ramGauge->setRange(0, 100);
    m_ramGauge->setColor(QColor(160, 80, 240));       // purple
    m_ramGauge->setWarnThreshold(0.70);
    m_ramGauge->setCritThreshold(0.90);
    m_ramGauge->setValue(0);
}

void DashboardWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("  🔄 500ms refresh  |  📁 Logging to: telemetry.log");
    statusBar()->addPermanentWidget(m_statusLabel, 1);
}

// ── Public Slots ─────────────────────────────────────────────────────────────

void DashboardWindow::setCpuLoad(double pct)
{
    m_cpuLoadGauge->setValue(pct);
}

void DashboardWindow::setCpuTemp(double celsius)
{
    m_cpuTempGauge->setValue(celsius);
}

void DashboardWindow::setRamUsage(double pct)
{
    m_ramGauge->setValue(pct);
}

void DashboardWindow::onConnected(const QString& host)
{
    m_connBadge->setText("● LIVE");
    m_connBadge->setProperty("connected", true);
    m_connBadge->style()->unpolish(m_connBadge);
    m_connBadge->style()->polish(m_connBadge);
    m_hostLabel->setText(QString("vSOME/IP | %1").arg(host));
}

void DashboardWindow::onDisconnected()
{
    m_connBadge->setText("● DISCONNECTED");
    m_connBadge->setProperty("connected", false);
    m_connBadge->style()->unpolish(m_connBadge);
    m_connBadge->style()->polish(m_connBadge);
    m_hostLabel->setText("vSOME/IP | Connecting...");
    m_cpuLoadGauge->setValue(0);
    m_cpuTempGauge->setValue(0);
    m_ramGauge->setValue(0);
}
