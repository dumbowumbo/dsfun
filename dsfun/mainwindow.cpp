#include "mainwindow.h"
#include "../ui/ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
    //if(timer != NULL)
        //delete timer;
}

void MainWindow::on_attachButton_clicked()
{
    // Offsets for HP
    //DWORD HpOffsets[] = { 0x74, 0xB8, 0x08, 0x14, 0x04, 0x04, 0xFC };
    //DWORD MaxHpOffsets[] = { 0x4C, 0x84, 0x40, 0x78, 0x1C, 0x104, 0x104 };
    // Offsets for deaths
    //DWORD DeathOffsets[] = { 0x74, 0xC, 0x08, 0x08, 0x10, 0x04, 0x1E0 };

    DWORD HpOffsets[2] = { 0x74, 0xFC };
    DWORD MaxHpOffsets[2] = { 0x74, 0x104 };
    DWORD DeathOffsets[3] = { 0x74, 0x378, 0x1A0 };

    //DWORD HpOffsets[2] = { 0xD0, 0x168 };
    //DWORD MaxHpOffsets[2] = { 0xD0, 0x170 };
    //DWORD DeathOffsets[3] = { 0xD0, 0x268, 0x364 };

    wchar_t PrName[] = L"DarkSoulsII.exe";
    int hOffSize = sizeof(HpOffsets) / sizeof(DWORD);
    int mhOffSize = sizeof(MaxHpOffsets) / sizeof(DWORD);
    int dOffSize = sizeof(DeathOffsets) / sizeof(DWORD);
    
    gameInstance->init(PrName, HpOffsets, MaxHpOffsets, DeathOffsets, hOffSize, mhOffSize, dOffSize); //0x01150414
    int ret = gameInstance->attachProc();
    if (!ret) {
        gameInstance->setAddresses();
        ui->attachLabel->setText("Attached.");
        attachFlag = 1;
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
    ix::HttpResponsePtr out;
    ix::HttpClient httpClient;
    ix::HttpRequestArgsPtr args = httpClient.createRequest();
    std::string port = ui->portSpinBox->text().toStdString();
    std::string url = "http://127.0.0.1:";
    std::string header = "/deviceconfig";
    std::string fullurl = url + port + header;

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

    ui->devicelistView->setSelectionMode(QAbstractItemView::SingleSelection);
}

void MainWindow::on_goButton_clicked() {
    QString selectedTexts;
    QModelIndexList selectedIndexes;

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
            if (ui->devicelistView->selectionModel() && ui->devicelistView->selectionModel()->selectedIndexes().size() > 0) {
                selectedIndexes = ui->devicelistView->selectionModel()->selectedIndexes();
                selectedTexts = selectedIndexes[0].data(Qt::DisplayRole).toString();

                std::string port = ui->portSpinBox->text().toStdString();
                std::string url = "http://127.0.0.1:";
                std::string header = "/haptic";
                std::string fullurl = url + port + header;
                int mode;

                if (ui->receivedradioButton->isChecked()) mode = 0;
                else if (ui->dealtradioButton->isChecked()) mode = 1;
                else mode = -1;

                std::string token = selectedTexts.toStdString().substr(0, selectedTexts.toStdString().find(";"));

                timer = std::make_shared<timedRead>(fullurl, token, mode, gameInstance);
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

std::vector<std::string> MainWindow::splitString(const std::string& str) {
    std::vector<std::string> tokens;

    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, '\n')) {
        tokens.push_back(token);
    }

    return tokens;
}
