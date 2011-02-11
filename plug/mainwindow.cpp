#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // load window size
    QSettings settings;
    restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
    restoreState(settings.value("mainWindowState").toByteArray());

    // create child objects
    amp_ops = new Mustang();
    amp = new Amplifier(this);
    effect1 = new Effect(this, 0);
    effect2 = new Effect(this, 1);
    effect3 = new Effect(this, 2);
    effect4 = new Effect(this, 3);

    about_window = new About(this);
    save = new SaveOnAmp(this);
    load = new LoadFromAmp(this);

    // connect buttons to slots
    connect(ui->Amplifier, SIGNAL(clicked()), amp, SLOT(show()));
    connect(ui->EffectButton1, SIGNAL(clicked()), effect1, SLOT(show()));
    connect(ui->EffectButton2, SIGNAL(clicked()), effect2, SLOT(show()));
    connect(ui->EffectButton3, SIGNAL(clicked()), effect3, SLOT(show()));
    connect(ui->EffectButton4, SIGNAL(clicked()), effect4, SLOT(show()));
    connect(ui->actionConnect, SIGNAL(triggered()), this, SLOT(start_amp()));
    connect(ui->actionDisconnect, SIGNAL(triggered()), this, SLOT(stop_amp()));
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionAbout, SIGNAL(triggered()), about_window, SLOT(open()));
    connect(ui->actionSave_to_amplifier, SIGNAL(triggered()), save, SLOT(open()));
    connect(ui->action_Load_from_amplifier, SIGNAL(triggered()), load, SLOT(open()));

    // shortcut to activate buttons
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_A), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(enable_buttons()));

    check_for_updates();
}

MainWindow::~MainWindow()
{
    QSettings settings;
    settings.setValue("mainWindowGeometry", saveGeometry());
    settings.setValue("mainWindowState", saveState());
    delete amp_ops;    // stop the communication before exiting
    delete ui;
}

void MainWindow::start_amp()
{
    int x;
    struct amp_settings amplifier_set;
    struct fx_pedal_settings effects_set[4];
    char name[32], /*title[40],*/ names[24][32];

    ui->statusBar->showMessage(tr("Connecting..."), 0);
    x = amp_ops->start_amp(names, name, &amplifier_set, effects_set);    // request initialization of communication

    if(x != 0)    // if request succeded
    {
        if(x == -100)
            ui->statusBar->showMessage(tr("Suitable device not found!"), 5000);
        else
            ui->statusBar->showMessage(QString(tr("Error: %1")).arg(x), 5000);
        return;
    }

    load->load_names(names);
    save->load_names(names);

    if(name[0] == 0x00)
        setWindowTitle(QString(tr("PLUG: NONE")));
    else
        setWindowTitle(QString(tr("PLUG: %1")).arg(name));

    amp->load(amplifier_set);
    for(int i = 0; i < 4; i++)
    {
        switch(effects_set[i].fx_slot)
        {
        case 0x00:
        case 0x04:
            effect1->load(effects_set[i]);
        case 0x01:
        case 0x05:
            effect2->load(effects_set[i]);
        case 0x02:
        case 0x06:
            effect3->load(effects_set[i]);
        case 0x03:
        case 0x07:
            effect4->load(effects_set[i]);
        }
    }

    // activate buttons
    ui->Amplifier->setDisabled(false);
    ui->EffectButton1->setDisabled(false);
    ui->EffectButton2->setDisabled(false);
    ui->EffectButton3->setDisabled(false);
    ui->EffectButton4->setDisabled(false);
    ui->actionSave_to_amplifier->setDisabled(false);
    ui->action_Load_from_amplifier->setDisabled(false);
    ui->statusBar->showMessage(tr("Connected"), 3000);    // show message on the status bar
}

void MainWindow::stop_amp()
{
    int x;

    x = amp_ops->stop_amp();
    if(x == 0)    // if request succeded
    {
        // deactivate buttons
        ui->Amplifier->setDisabled(true);
        ui->EffectButton1->setDisabled(true);
        ui->EffectButton2->setDisabled(true);
        ui->EffectButton3->setDisabled(true);
        ui->EffectButton4->setDisabled(true);
        ui->actionSave_to_amplifier->setDisabled(true);
        ui->action_Load_from_amplifier->setDisabled(true);
        ui->statusBar->showMessage(tr("Disconnected"), 3000);    // show message on the status bar
    }
    else    // if request failed
        ui->statusBar->showMessage(QString(tr("Error: %1")).arg(x), 5000);
}

// pass the message to the amp
int MainWindow::set_effect(struct fx_pedal_settings pedal)
{
    return amp_ops->set_effect(pedal);
}

int MainWindow::set_amplifier(struct amp_settings settings)
{
    return amp_ops->set_amplifier(settings);
}

int MainWindow::save_on_amp(char *name, int slot)
{
    int ret;

    ret = amp_ops->save_on_amp(name, slot);

    if(name[0] == 0x00)
        setWindowTitle(QString(tr("PLUG: NONE")));
    else
        setWindowTitle(QString(tr("PLUG: %1")).arg(name));

    return ret;
}

int MainWindow::load_from_amp(int slot)
{
    struct amp_settings amplifier_set;
    struct fx_pedal_settings effects_set[4];
    char name[32];

    amp_ops->load_memory_bank(slot, name, &amplifier_set, effects_set);

    if(name[0] == 0x00)
        setWindowTitle(QString(tr("PLUG: NONE")));
    else
        setWindowTitle(QString(tr("PLUG: %1")).arg(name));

    amp->load(amplifier_set);
    for(int i = 0; i < 4; i++)
    {
        switch(effects_set[i].fx_slot)
        {
        case 0x00:
        case 0x04:
            effect1->load(effects_set[i]);
        case 0x01:
        case 0x05:
            effect2->load(effects_set[i]);
        case 0x02:
        case 0x06:
            effect3->load(effects_set[i]);
        case 0x03:
        case 0x07:
            effect4->load(effects_set[i]);
        }
    }

    return 0;
}

// activate buttons
void MainWindow::enable_buttons(void)
{
    ui->Amplifier->setDisabled(false);
    ui->EffectButton1->setDisabled(false);
    ui->EffectButton2->setDisabled(false);
    ui->EffectButton3->setDisabled(false);
    ui->EffectButton4->setDisabled(false);
    ui->actionSave_to_amplifier->setDisabled(false);
    ui->action_Load_from_amplifier->setDisabled(false);
}

void MainWindow::check_for_updates()
{
    QNetworkAccessManager *qnam = new QNetworkAccessManager(this);
    reply = qnam->get(QNetworkRequest((QUrl)"http://piorekf.org/plug/VERSION"));
    connect(reply, SIGNAL(readyRead()), this, SLOT(httpReadyRead()));
}

void MainWindow::httpReadyRead()
{
    if(reply->readAll()>VERSION)
    {
        QLabel *label = new QLabel("Update available!", this);
        ui->statusBar->addWidget(label);
        QMessageBox::information(this, "Update", "Update available!\nCheck homepage for new version.");
    }
}

// save window size on close
//void MainWindow::closeEvent(QCloseEvent *event)
//{
//    QSettings settings;
//    settings.setValue("mainWindowGeometry", saveGeometry());
//    settings.setValue("mainWindowState", saveState());
//}

void MainWindow::change_name(int slot, QString *name)
{
    load->change_name(slot, name);
}
