#include "downloadbottomareawidget.h"
#include "ui_downloadapplication.h"
#include "downloadapplication.h"
#include "downloadsystemtraymenu.h"
#include "downloadwindowextras.h"
#include "downloadnumberutils.h"
#include "downloadnetworkspeedtestthread.h"
#include "downloaduiobject.h"
#include "downloadwidgetutils.h"
#include "downloadbackgroundmanager.h"
#include "downloadsettingmanager.h"
#include "downloadrightareawidget.h"

DownloadBottomAreaWidget *DownloadBottomAreaWidget::m_instance = nullptr;

DownloadBottomAreaWidget::DownloadBottomAreaWidget(QWidget *parent)
    : QWidget(parent)
{
    m_instance = this;
    m_systemCloseConfig = false;
    m_toolPopupMenu = nullptr;

    createSystemTrayIcon();

    m_windowExtras = new DownloadWindowExtras(parent);
    m_speedThread = new DownloadNetworkSpeedTestThread(this);

    connect(m_speedThread, SIGNAL(networkData(ulong,ulong)), SLOT(updateNetworkData(ulong,ulong)));
}

DownloadBottomAreaWidget::~DownloadBottomAreaWidget()
{
    m_speedThread->stopAndQuitThread();
    delete m_toolPopupMenu;
    delete m_systemTrayMenu;
    delete m_systemTray;
    delete m_windowExtras;
    delete m_speedThread;
}

QString DownloadBottomAreaWidget::getClassName()
{
    return staticMetaObject.className();
}

DownloadBottomAreaWidget *DownloadBottomAreaWidget::instance()
{
    return m_instance;
}

void DownloadBottomAreaWidget::setupUi(Ui::DownloadApplication* ui)
{
    m_ui = ui;
    m_speedThread->start();

    ui->resizeLabelWidget->setPixmap(QPixmap(":/tiny/lb_resize_normal"));

    ui->expandButton->setLabelIcon(":/functions/lb_left");

    ui->downloadSettingButton->setLabelText(tr("Download"));
    ui->downloadSettingButton->setLabelIcon(":/functions/lb_order");

    ui->downloadPlanButton->setLabelText(tr("Plan"));
    ui->downloadPlanButton->setLabelIcon(":/functions/lb_plan");

    ui->downloadSpeedButton->setLabelText("--", "--");

    connect(ui->downloadSettingButton, SIGNAL(mouseClicked()), SLOT(downloadSettingButtonClicked()));
    connect(ui->downloadPlanButton, SIGNAL(mouseClicked()), SLOT(downloadPlanButtonClicked()));
    connect(ui->expandButton, SIGNAL(clicked()), SLOT(expandButtonClicked()));
}

void DownloadBottomAreaWidget::showMessage(const QString &title, const QString &text)
{
    m_systemTray->showMessage(title, text);
}

#if defined DOWNLOAD_DEBUG && defined Q_OS_WIN && defined DOWNLOAD_WINEXTRAS
void DownloadBottomAreaWidget::setValue(int value) const
{
    m_windowExtras->setValue(value);
}

void DownloadBottomAreaWidget::setRange(int min, int max) const
{
    m_windowExtras->setRange(min, max);
}
#endif

void DownloadBottomAreaWidget::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
        case QSystemTrayIcon::DoubleClick:
            break;
        case QSystemTrayIcon::Trigger:
            {
                DownloadApplication *w = DownloadApplication::instance();
                if(w->isMinimized() || w->isHidden())
                {
                    w->showNormal();
                    w->activateWindow();
                }
                break;
            }
        default:
            break;
    }
}

void DownloadBottomAreaWidget::updateNetworkData(ulong upload, ulong download)
{
    const QString &up = DownloadUtils::Number::speed2Label(upload);
    const QString &down = DownloadUtils::Number::speed2Label(download);
    m_ui->downloadSpeedButton->setLabelText(up, down);
}

void DownloadBottomAreaWidget::downloadSettingButtonClicked()
{
    delete m_toolPopupMenu;
    m_toolPopupMenu = new QMenu(this);
    m_toolPopupMenu->setStyleSheet(DownloadUIObject::MMenuStyle02);

    QPixmap pix(17, 17);
    pix.fill(M_BACKGROUND_PTR->getMBackgroundColor());
    DownloadUtils::Widget::fusionPixmap(pix, QPixmap(":/contextMenu/lb_protection"), QPoint(2, 2));
    m_toolPopupMenu->addAction(QIcon(pix), tr("Speed Protection"));
    pix.fill(M_BACKGROUND_PTR->getMBackgroundColor());
    DownloadUtils::Widget::fusionPixmap(pix, QPixmap(":/contextMenu/lb_custom"), QPoint(2, 2));
    m_toolPopupMenu->addAction(QIcon(pix), tr("Custom Speed Limit"));
    m_toolPopupMenu->addSeparator();
    pix.fill(M_BACKGROUND_PTR->getMBackgroundColor());
    DownloadUtils::Widget::fusionPixmap(pix, QPixmap(":/contextMenu/lb_setting"), QPoint(2, 2));
    m_toolPopupMenu->addAction(QIcon(pix), tr("More Setting..."));

    m_ui->downloadSettingButton->setMenu(m_toolPopupMenu);
    m_ui->downloadSettingButton->setPopupMode(QToolButton::InstantPopup);
}

void DownloadBottomAreaWidget::downloadPlanButtonClicked()
{
    delete m_toolPopupMenu;
    m_toolPopupMenu = new QMenu(this);
    m_toolPopupMenu->setStyleSheet(DownloadUIObject::MMenuStyle02);
    m_toolPopupMenu->addAction(tr("Spare"));
    m_toolPopupMenu->addAction(tr("Add Plan"));
    m_toolPopupMenu->addAction(tr("Delete Plan"))->setEnabled(false);

    QMenu *download = m_toolPopupMenu->addMenu(tr("Downloaded"));
    download->addAction(tr("Close"));
    download->addAction(tr("Locked"));
    download->addAction(tr("Sleep"));
    download->addAction(tr("Quit"));

    m_ui->downloadPlanButton->setMenu(m_toolPopupMenu);
    m_ui->downloadPlanButton->setPopupMode(QToolButton::InstantPopup);
}

void DownloadBottomAreaWidget::expandButtonClicked()
{
    bool con = m_windowExtras->isDisableBlurBehindWindow();

    if(con)
    {
        M_SETTING_PTR->setValue(DownloadSettingManager::ExpandModeChoiced, 135);
        m_ui->leftWidget->setMinimumWidth(70);
        m_ui->leftWidget->setMaximumWidth(70);
        m_ui->expandButton->setLabelIcon(":/functions/lb_right");
        m_ui->fucntionListWidget->resizeMode(true);
    }
    else
    {
        M_SETTING_PTR->setValue(DownloadSettingManager::ExpandModeChoiced, 0);
        m_ui->leftWidget->setMinimumWidth(205);
        m_ui->leftWidget->setMaximumWidth(205);
        m_ui->expandButton->setLabelIcon(":/functions/lb_left");
        m_ui->fucntionListWidget->resizeMode(false);
    }

    DownloadRightAreaWidget::instance()->resizeWindow();

    m_windowExtras->disableBlurBehindWindow( !con );
}

void DownloadBottomAreaWidget::createSystemTrayIcon()
{
    m_systemTray = new QSystemTrayIcon(DownloadApplication::instance());
    m_systemTray->setIcon(QIcon(":/image/lb_player_logo"));
    m_systemTray->setToolTip(tr("TTKDownloader"));

    m_systemTrayMenu = new DownloadSystemTrayMenu(DownloadApplication::instance());

    m_systemTray->setContextMenu(m_systemTrayMenu);
    m_systemTray->show();
    connect(m_systemTray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                          SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
}
