#include "fontawindow.h"
#include "ui_fontawindow.h"

#include "fontawidgets.h"
#include "sampler.h"
#include "filterwizard.h"

#include <QTextStream>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QColorDialog>

#include <QDebug>

const Version FontaWindow::versionNumber = Version(0, 4, 2);

void FontaWindow::initAlignButton(QPushButton*& button, int size, QButtonGroup* buttonGroup, CStringRef iconPath)
{
    button = new QPushButton();
    button->setMinimumSize(QSize(size, size));
    button->setMaximumSize(QSize(size, size));
    button->setText(QString());
    button->setCheckable(true);
    button->setFlat(false);
    button->setIcon(QIcon(iconPath));
    buttonGroup->addButton(button);
}

FontaWindow::FontaWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::FontaWindow)
    , aboutDialog(NULL)
    , currentProjectFile("")
{
    ui->setupUi(this);
    ui->fontsListSplitter->setSizes(QList<int>() << 110 << 200);

    connect(ui->sizeBox->lineEdit(), SIGNAL(returnPressed()), this, SLOT(on_sizeBox_edited()));
    connect(ui->leadingBox->lineEdit(), SIGNAL(returnPressed()), this, SLOT(on_leadingBox_edited()));
    connect(ui->trackingBox->lineEdit(), SIGNAL(returnPressed()), this, SLOT(on_trackingBox_edited()));

    QButtonGroup* buttonGroup = new QButtonGroup(this);

    const int buttonSize = 30;
    initAlignButton(topLeftButton, buttonSize, buttonGroup, ":/pic/left.png");
    initAlignButton(topCenterButton, buttonSize, buttonGroup ,":/pic/center.png");
    initAlignButton(topRightButton, buttonSize, buttonGroup, ":/pic/right.png");
    initAlignButton(topJustifyButton, buttonSize, buttonGroup, ":/pic/justify.png");

    ui->horizontalLayout->insertWidget(2, topLeftButton);
    ui->horizontalLayout->insertWidget(3, topCenterButton);
    ui->horizontalLayout->insertWidget(4, topRightButton);
    ui->horizontalLayout->insertWidget(5, topJustifyButton);

    connect(topLeftButton, SIGNAL(clicked(bool)), this, SLOT(on_topLeftButton_clicked()));
    connect(topCenterButton, SIGNAL(clicked(bool)), this, SLOT(on_topCenterButton_clicked()));
    connect(topRightButton, SIGNAL(clicked(bool)), this, SLOT(on_topRightButton_clicked()));
    connect(topJustifyButton, SIGNAL(clicked(bool)), this, SLOT(on_topJustifyButton_clicked()));

    QStringList filterItems;
    for(int i = FilterMode::Start; i<FilterMode::End; ++i) {
        filterItems << FilterMode::toString(static_cast<FilterMode::type>(i));
    }
    ui->filterBox->addItems(filterItems);

    fontFinderEdit = new FontaFilterEdit(ui->fontsList, ui->fontsListLayoutWidget);
    fontFinderEdit->setObjectName(QStringLiteral("fontFinderEdit"));
    ui->fontsListLayout->insertWidget(1, fontFinderEdit);

    ui->tabWidget->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tabWidget->setTabsClosable(false);
    ui->tabWidget->tabBar()->setStyleSheet("QTabBar::tab { height: 27px; }"
                                           "QTabBar::close-button {image: url(:/pic/closeTab.png); }"
                                           "QTabBar::close-button:hover {image: url(:/pic/closeTabHover.png); }"
                                           );
    connect(ui->tabWidget, SIGNAL(tabCloseRequested(int)), SLOT(promptedCloseTab(int)));
    connect(ui->tabWidget, SIGNAL(tabCloseRequested(int)), SLOT(changeAddTabButtonGeometry()));
    connect(ui->tabWidget->tabBar(), SIGNAL(tabBarDoubleClicked(int)), this, SLOT(renameTab(int)));
    connect(ui->tabWidget->tabBar(), SIGNAL(tabMoved(int,int)), this, SLOT(onTabsMove(int, int)));
    connect(ui->tabWidget->tabBar(), SIGNAL(customContextMenuRequested(QPoint)), SLOT(showTabsContextMenu(const QPoint &)));

    addTab();

    addTabButton = new QPushButton(ui->tabWidget->tabBar());
    connect(addTabButton, SIGNAL(clicked(bool)), this, SLOT(addTab()));
    connect(addTabButton, SIGNAL(clicked(bool)), this, SLOT(changeAddTabButtonGeometry()));
    changeAddTabButtonGeometry();
}

void FontaWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    changeAddTabButtonGeometry();
}

void FontaWindow::changeAddTabButtonGeometry()
{
    QRect r = ui->tabWidget->tabBar()->tabRect(workAreas.length()-1);
    const int padding = 2;
    const int sz = r.height()-3*padding;
    addTabButton->setGeometry(r.x() + r.width() + padding, padding, sz, sz);
}

void FontaWindow::onTabsMove(int from, int to)
{
    FontaWorkArea* toMove = workAreas.at(from);
    workAreas.removeAt(from);
    workAreas.insert(to, toMove);
}

void FontaWindow::showTabsContextMenu(const QPoint &point)
{
    if (point.isNull()) {
        return;
    }

    int length = workAreas.size();
    if(length <= 1) {
        return;
    }

    int tabIndex = ui->tabWidget->tabBar()->tabAt(point);
    ui->tabWidget->setCurrentIndex(tabIndex);

    QMenu menu(this);

    QAction remove("Close Other Tabs", this);
    connect(&remove, SIGNAL(triggered(bool)), this, SLOT(closeOtherTabs()));
    connect(&remove, SIGNAL(triggered(bool)), this, SLOT(changeAddTabButtonGeometry()));
    menu.addAction(&remove);

    menu.exec(ui->tabWidget->tabBar()->mapToGlobal(point));
}

void FontaWindow::addTab(bool empty)
{
    int id = workAreas.length();

    QWidget* tab = new QWidget();
    QHBoxLayout* horizontalLayout = new QHBoxLayout(tab);
    horizontalLayout->setSpacing(0);
    horizontalLayout->setContentsMargins(11, 11, 11, 11);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    currWorkArea = new FontaWorkArea(id, tab, Sampler::getName());

    if(!empty) {
        currWorkArea->createSample();
    }
    horizontalLayout->addWidget(currWorkArea);
    ui->tabWidget->addTab(tab, currWorkArea->name());

    workAreas.push_back(currWorkArea);

    currField = currWorkArea->currField();

    makeFieldsConnected();
    ui->tabWidget->setCurrentIndex(id);

    if(!empty) {
        currField->setFocus();
    }

    if(ui->tabWidget->count() > 1) {
        ui->tabWidget->setTabsClosable(true);
    }
}

void FontaWindow::promptedCloseTab(int i)
{
    QMessageBox msgBox;
    msgBox.setText("Delete " + workAreas[i]->name() + " tab.\nSure?");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    int ret = msgBox.exec();

    if (ret == QMessageBox::Ok) {
        closeTab(i);
    }
}

void FontaWindow::closeTab(int id)
{
    delete workAreas[id];
    workAreas.removeAt(id);
    ui->tabWidget->removeTab(id);

    for(int i = id; i<workAreas.length(); ++i) {
        workAreas[i]->setId(i);
    }
    currWorkArea = id < workAreas.length() ? workAreas[id] : workAreas.last();

    if(ui->tabWidget->count() == 1) {
        ui->tabWidget->setTabsClosable(false);
    }
}

void FontaWindow::closeOtherTabs()
{
    QMessageBox msgBox;
    msgBox.setText("Delete all tabs except " + currWorkArea->name() + "\nSure?");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    int ret = msgBox.exec();

    if (ret != QMessageBox::Ok) {
        return;
    }

    FontaWorkArea* onlyArea = currWorkArea;

    for(int i = 0; i<workAreas.size(); ++i) {
        if(workAreas[i] != onlyArea) {
            closeTab(i);
            --i;
        }
    }
}

void FontaWindow::renameTab(int id)
{
    RenameTabEdit* edit = new RenameTabEdit(ui->tabWidget, workAreas[id], ui->tabWidget->tabBar());
    connect(edit, SIGNAL(applied()), this, SLOT(changeAddTabButtonGeometry()));
    edit->show();
}

FontaWindow::~FontaWindow()
{
    delete ui;
}

void FontaWindow::makeFieldConnected(FontaField* field) {
    connect(field, SIGNAL(focussed(FontaField*)), this, SLOT(on_currentFieldChanged(FontaField*)));
}

void FontaWindow::makeFieldsConnected() {
    for(int i = 0; i<currWorkArea->fieldCount(); ++i) {
        FontaField* field = (*currWorkArea)[i];
        makeFieldConnected(field);
    }
}

void FontaWindow::on_addFieldButton_clicked()
{
    FontaField* field = currWorkArea->addField();
    makeFieldConnected(field);

    updateAddRemoveButtons();
}

void FontaWindow::on_removeFieldButton_clicked()
{
    int count = currWorkArea->fieldCount();
    if(currField->id() == count-1) {
        (*currWorkArea)[count-2]->setFocus();
    }

    currWorkArea->popField();
    updateAddRemoveButtons();
}

void FontaWindow::on_currentFieldChanged(FontaField* field)
{
    currField = field;
    CStringRef family = currField->fontFamily();

    // show family
    fontFinderEdit->setText(family);

    QList<QListWidgetItem*> items = ui->fontsList->findItems(family, Qt::MatchExactly);
    if(items.size() > 0) {
        ui->fontsList->setCurrentItem(items[0]);
        ui->fontsList->scrollToItem(items[0], QAbstractItemView::PositionAtCenter);
    }

    // show size
    ui->sizeBox->lineEdit()->setText(QString::number(currField->fontSize()) + " pt");

    // show style
    ui->styleBox->setCurrentIndex(ui->styleBox->findText(currField->fontStyle()));

    // show leading
    float lead = currField->leading();
    if(lead == inf()) {
        ui->leadingBox->lineEdit()->setText("Auto");
    } else {
        ui->leadingBox->lineEdit()->setText(QString::number(lead) + " pt");
    }


    // show tracking
    ui->trackingBox->lineEdit()->setText(QString::number(currField->tracking()));

    // show alignment
    switch(currField->textAlignment()) {
        default:
        case (Qt::AlignLeft): topLeftButton->setChecked(true); break;
        case (Qt::AlignHCenter): topCenterButton->setChecked(true); break;
        case (Qt::AlignRight): topRightButton->setChecked(true); break;
        case (Qt::AlignJustify): topJustifyButton->setChecked(true); break;
    }
}

void FontaWindow::on_fontsList_currentTextChanged(const QString &family)
{
    /*if(family.isEmpty()) {
        return;
    }*/

    fontFinderEdit->setText(family);

    ui->styleBox->clear();
    ui->styleBox->addItems(fontaDB().styles(family));

    currField->setFontFamily(family);
    ui->styleBox->setCurrentText(currField->fontStyle());
}

void FontaWindow::on_sizeBox_edited()
{
    on_sizeBox_activated(ui->sizeBox->lineEdit()->text());
}

void FontaWindow::on_sizeBox_activated(const QString &arg1)
{
    float val = strtof(arg1.toStdString().c_str(), nullptr);
    currField->setFontSize(val);
}

void FontaWindow::on_filterBox_currentIndexChanged(int index)
{
    if(ui->filterBox->currentText() == "Custom") {
        return;
    } else {
        int lastIndex = ui->filterBox->count()-1;
        if(ui->filterBox->itemText(lastIndex) == "Custom") {
            ui->filterBox->removeItem(lastIndex);
        }
    }

    ui->fontsList->clear();

    bool (FontaDB::*goodFont)(CStringRef) const;
    switch(index) {
        default:
        case FilterMode::ALL:        goodFont = &FontaDB::isAnyFont; break;
        case FilterMode::CYRILLIC:   goodFont = &FontaDB::isCyrillic; break;
        case FilterMode::SERIF:      goodFont = &FontaDB::isSerif; break;
        case FilterMode::SANS_SERIF: goodFont = &FontaDB::isSansSerif; break;
        case FilterMode::MONOSPACE:  goodFont = &FontaDB::isMonospaced; break;
        case FilterMode::SCRIPT:     goodFont = &FontaDB::isScript; break;
        case FilterMode::DECORATIVE: goodFont = &FontaDB::isDecorative; break;
        case FilterMode::SYMBOLIC:   goodFont = &FontaDB::isSymbolic; break;
    }

    for (CStringRef family : fontaDB().families()) {

        if(!(fontaDB().*goodFont)(family)) {
            continue;
        }

        QListWidgetItem* item = new QListWidgetItem(family);

#ifdef FONTA_DETAILED_DEBUG
        FullFontInfo info = fontaDB().getFullFontInfo(family);
        QString detail;
        QTextStream ss(&detail);

        QString pad("      ");
        ss << "Qt:\n";
        if(info.qtInfo.cyrillic) ss << pad << "Cyrillic\n";
        if(info.qtInfo.symbolic) ss << pad << "Symbolic\n";
        if(info.qtInfo.monospaced) ss << pad << "Monospaced\n";

        if(info.TTFExists) {
            ss << "TTF:\n";
            ss << pad << "Family:     " << FamilyClass::toString(info.fontaTFF.familyClass) << "\n";
            ss << pad << "Family Sub: " << info.fontaTFF.familySubClass << "\n";
            ss << pad << "Panose: " << info.fontaTFF.panose.getNumberAsString() << "\n";
            if(info.fontaTFF.cyrillic) ss << pad << "Cyrillic\n";
            if(info.fontaTFF.monospaced) ss << pad << "Monospaced\n";
            ss << pad << "Files: " << info.fontaTFF.files.toList().join(' ') << "\n";
            if(!info.fontaTFF.linkedFonts.isEmpty()) {
                ss << pad << "Linked fonts: " << info.fontaTFF.linkedFonts.toList().join(' ');
            }
        } else {
            qWarning() << family << qPrintable("doesn't have TTF");
        }

        if(detail.endsWith('\n') ) {
            detail.truncate(detail.size()-1);
        }

        item->setToolTip(detail);
#endif

        ui->fontsList->addItem(item);
    }
}

void FontaWindow::on_styleBox_activated(CStringRef style)
{
    currField->setPreferableFontStyle(style);
}

void FontaWindow::on_topLeftButton_clicked()       { currField->alignText(Qt::AlignLeft); }
void FontaWindow::on_topCenterButton_clicked()     { currField->alignText(Qt::AlignHCenter); }
void FontaWindow::on_topRightButton_clicked()      { currField->alignText(Qt::AlignRight); }
void FontaWindow::on_topJustifyButton_clicked()    { currField->alignText(Qt::AlignJustify); }

void FontaWindow::on_leadingBox_edited()
{
    on_leadingBox_activated(ui->leadingBox->lineEdit()->text());
}

void FontaWindow::on_leadingBox_activated(CStringRef arg1)
{
    //int val = strtol(arg1.toStdString().c_str(), nullptr, 10);

    float val = inf();
    if(arg1 != "Auto") {
        val = strtof(arg1.toStdString().c_str(), nullptr);
    }

    currField->setLeading(val);
}

void FontaWindow::on_trackingBox_edited()
{
    on_trackingBox_activated(ui->trackingBox->lineEdit()->text());
}

void FontaWindow::on_trackingBox_activated(CStringRef arg1)
{
    int val = strtol(arg1.toStdString().c_str(), nullptr, 10);
    currField->setTracking(val);
}

void FontaWindow::save(CStringRef fileName) const
{
    QJsonObject json;

    QJsonObject version;
    version["major"] = versionNumber.major;
    version["minor"] = versionNumber.minor;
    version["build"] = versionNumber.build;
    json["version"] = version;

    QJsonArray workAreasArr;
    for(auto workArea : workAreas) {
        QJsonObject json;
        workArea->save(json);
        workAreasArr.append(json);
    }

    json["workAreas"] = workAreasArr;
    json["currWorkArea"] = currWorkArea->id();

    QFile saveFile(fileName);

    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return;
    }

    QJsonDocument saveDoc(json);
    saveFile.write(saveDoc.toJson());
}

void FontaWindow::on_actionSave_as_triggered()
{
    QString filename =
            QFileDialog::getSaveFileName(this, "Save Fonta", QDir::homePath(), "Fonta files (*.fonta)");

    if(!filename.isNull()) {
        save(filename);
        setCurrentProjectFile(filename);
    }
}

void FontaWindow::load(CStringRef fileName)
{
    QFile loadFile(fileName);

    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open save file.");
        return;
    }

    clearWorkAreas();

    QByteArray saveData = loadFile.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    QJsonObject json = loadDoc.object();

    QJsonArray workAreas = json["workAreas"].toArray();
    for(const QJsonValue& workArea : workAreas) {
        addTab(true);
        QJsonObject areaJson = workArea.toObject();
        currWorkArea->load(areaJson);
        makeFieldsConnected();
        ui->tabWidget->setTabText(currWorkArea->id(), currWorkArea->name());
    }

    int workAreaId = json["currWorkArea"].toInt(0);
    ui->tabWidget->setCurrentIndex(workAreaId);
}

void FontaWindow::on_actionOpen_triggered()
{
    QString filename =
            QFileDialog::getOpenFileName(this, "Open Fonta", QDir::homePath(), "Fonta files (*.fonta)");

    if(!filename.isNull()) {
        load(filename);
    }

    setCurrentProjectFile(filename);
    changeAddTabButtonGeometry();
}

void FontaWindow::on_actionSave_triggered()
{
    if(currentProjectFile.isEmpty()) {
        on_actionSave_as_triggered();
    } else {
        save(currentProjectFile);
    }
}

void FontaWindow::setCurrentProjectFile(CStringRef filename)
{
    currentProjectFile = filename;
    QFileInfo fileInfo(filename);
    setWindowTitle("Fonta - " + fileInfo.fileName());
}

void FontaWindow::resetCurrentProjectFile()
{
    currentProjectFile.clear();
    setWindowTitle("Fonta");
}

void FontaWindow::clearWorkAreas()
{
    int prevSize = workAreas.size();
    for(int i = 0; i<prevSize; ++i) {
        QWidget* w = ui->tabWidget->widget(0);
        w->deleteLater();
    }
    this->workAreas.clear();
    ui->tabWidget->clear();
}

void FontaWindow::on_actionNew_triggered()
{
    resetCurrentProjectFile();
    clearWorkAreas();
    addTab();
    changeAddTabButtonGeometry();
}

void FontaWindow::updateAddRemoveButtons()
{
    ui->addFieldButton->setEnabled(currWorkArea->fieldCount() < 9);
    ui->removeFieldButton->setEnabled(currWorkArea->fieldCount() > 1);
}

void FontaWindow::setCurrWorkArea(int id)
{
    if(workAreas.isEmpty()) {
        return;
    }

    currWorkArea = workAreas[id];
    if(currWorkArea->fieldCount()) {
        currField = currWorkArea->currField();
        currField->setFocus();
    }
}

void FontaWindow::on_tabWidget_currentChanged(int index)
{
    if(index == -1) {
        return;
    }

    setCurrWorkArea(index);
    updateAddRemoveButtons();
}


void FontaWindow::on_actionAbout_triggered()
{
    if(aboutDialog == NULL) {
        aboutDialog = new About(versionNumber, this);
    }
    aboutDialog->show();
}

void FontaWindow::on_filterWizardButton_clicked()
{
    FilterWizard* w = new FilterWizard(this);
    w->exec();
}

void FontaWindow::filterFontList(const QStringList& l)
{
    ui->fontsList->clear();
    ui->fontsList->addItems(l);
    ui->filterBox->addItem("Custom");
    ui->filterBox->setCurrentText("Custom");
}

void FontaWindow::on_backColorButton_clicked()
{
    QColor c = QColorDialog::getColor(currField->sheet()["background-color"], this);

    if(c.isValid()) {
        currField->sheet().set("background-color", c.red(), c.green(), c.blue());
        currField->applySheet();
    }
}

void FontaWindow::on_textColorButton_clicked()
{
    QColor c = QColorDialog::getColor(currField->sheet()["color"], this);

    if(c.isValid()) {
        currField->sheet().set("color", c.red(), c.green(), c.blue());
        currField->applySheet();
    }
}
