#include "mainwindow.h"
#include "../ui/ui_mainwindow.h"

// Main window function
MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Add items to combo box for game type.
    ui->comboBox->addItem("DSII Base");
    ui->comboBox->addItem("DSII:SotFS");

    int val = ui->timeSlider->value();
    ui->timeLabel->setText(QStringLiteral("%1 s").arg((float)val / 10));

    val = ui->strSlider->value();
    ui->strLabel->setText(QStringLiteral("%1\%").arg(val));
}

// Main window destructor.
MainWindow::~MainWindow()
{
    delete ui;
}

// Procedure when attach button is clicked.
void MainWindow::on_attachButton_clicked()
{

    // Program name to attach to.
    wchar_t PrName[] = L"DarkSoulsII.exe";
    // Game type selection variable.
    int gameType = 0;

    // Determine which game and create shared instance of class so it would persist outside of this function.
    // InjAssertChar is for asserting whether there is an existing injection.
    switch (ui->comboBox->currentIndex()) {
    case 0:
        gameType = 0;
        gameInstance = std::make_shared<dsiiReroute>();
        gameInstance->InjAssertChar = 0xE8;
        break;
    case 1:
        gameType = 1;
        gameInstance = std::make_shared<dsiisotfsReroute>();
        gameInstance->InjAssertChar = 0x49;
        break;
    }
    
    // Init the game class.
    gameInstance->init(PrName);
    // Attach to game
    int ret = gameInstance->attachProc();
    // Check whether the right version of the game is running.
    if ((UINT64)std::numeric_limits<DWORD>::max() > gameInstance->BaseAddress && gameType) {
        QMessageBox msgBox;
        msgBox.setText("Attach fail. The game running is 32-bit, but you selected SotFS.");
        int ret = msgBox.exec();
    }
    else if ((UINT64)std::numeric_limits<DWORD>::max() < gameInstance->BaseAddress && !gameType) {
        QMessageBox msgBox;
        msgBox.setText("Attach fail. The game running is 64-bit, but you selected base DSII.");
        int ret = msgBox.exec();
    }
    else if (!ret) {
        // If the game is attached, grab the addresses for reading health and injection.
        gameInstance->setAddresses();
        ui->attachLabel->setText("Attached.");
        attachFlag = 1;
        // Make sure the game is not already injected.
        if (gameInstance->injAssert(gameInstance->InjAssertChar, gameInstance->HookAddr)) {
            ui->injectLabel->setText("Injected.");
            injectFlag = 1;
        }
        else {
            ui->injectLabel->setText("Not injected.");
            injectFlag = 0;
        }
    }
    else {
        QMessageBox msgBox;
        msgBox.setText("Attach fail. Make sure the game is running.");
        int ret = msgBox.exec();
    }
}

void MainWindow::on_scanButton_clicked() {
    // Set up variable for HTTP connection.
    ix::HttpResponsePtr out;
    ix::HttpClient httpClient;
    ix::HttpRequestArgsPtr args = httpClient.createRequest();
    std::string port = ui->portSpinBox->text().toStdString();
    std::string url = "http://127.0.0.1:";
    std::string header = "/deviceconfig";
    std::string fullurl = url + port + header;

    // Get the devices from buttplug lite and put them in to the list
    out = httpClient.get(fullurl,args);
    std::vector<std::string> tokens = splitString(out.get()->body);
    // Create model
    model = new QStringListModel(this);

    // Make data
    QStringList List;
    for (auto const& token : tokens) {
        List.append(QString::fromStdString(token));
    }

    // Populate our model
    model->setStringList(List);

    // Glue model and view together
    ui->devicelistView->setModel(model);

    // Make sure only single thing can be selected
    ui->devicelistView->setSelectionMode(QAbstractItemView::SingleSelection);
}

// Start button click function
void MainWindow::on_goButton_clicked() {
    QString selectedTexts;
    QModelIndexList selectedIndexes;

    // Inject if not injected.
    if (attachFlag && !injectFlag) {
        gameInstance->injectRoutine();
        ui->injectLabel->setText("Injected.");
        injectFlag = 1;
    }
    else if (!attachFlag) {
        QMessageBox msgBox;
        msgBox.setText("Please attach to process first.");
        int ret = msgBox.exec();
    }

    if (injectFlag && attachFlag) {
        if (!runFlag) {
            // Once injected, grab which device is selected
            if (ui->devicelistView->selectionModel() && ui->devicelistView->selectionModel()->selectedIndexes().size() > 0) {
                selectedIndexes = ui->devicelistView->selectionModel()->selectedIndexes();
                selectedTexts = selectedIndexes[0].data(Qt::DisplayRole).toString();

                // Set up variables for websocket
                std::string port = ui->portSpinBox->text().toStdString();
                std::string url = "http://127.0.0.1:";
                std::string header = "/haptic";
                std::string fullurl = url + port + header;
                int mode;

                // Check which mode is desired
                if (ui->receivedradioButton->isChecked()) mode = 0;
                else if (ui->dealtradioButton->isChecked()) mode = 1;
                else mode = -1;

                // Parse selected device string
                std::string token = selectedTexts.toStdString().substr(0, selectedTexts.toStdString().find(";"));

                // Create a timer which reads stats every 100 ms.
                timer = std::make_shared<timedRead>(fullurl, token, mode, gameInstance);
                int val = ui->timeSlider->value();
                timer->maxTime = val;
                val = ui->strSlider->value();
                timer->maxStr = val;

                ui->goButton->setText("Started");
                runFlag = 1;
            }

            else {
                QMessageBox msgBox;
                msgBox.setText("Please select a device.");
                int ret = msgBox.exec();
            }
        }
        else {
            ui->goButton->setText("Start");
            //delete timer;
            //timer = nullptr;
            runFlag = 0;
        }
    }
}

void MainWindow::on_timeSlider_valueChanged() {
    int val = ui->timeSlider->value();
    ui->timeLabel->setText(QStringLiteral("%1 s").arg((float)val/10));
    if(timer != nullptr)
        timer->maxTime = val;
}

void MainWindow::on_strSlider_valueChanged() {
    int val = ui->strSlider->value();
    ui->strLabel->setText(QStringLiteral("%1\%").arg(val));
    if (timer != nullptr)
        timer->maxStr = val;
}

// Helper function to split string.
std::vector<std::string> MainWindow::splitString(const std::string& str) {
    std::vector<std::string> tokens;

    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, '\n')) {
        tokens.push_back(token);
    }

    return tokens;
}
