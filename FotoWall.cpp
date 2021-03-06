/***************************************************************************
 *                                                                         *
 *   This file is part of the FotoWall project,                            *
 *       http://code.google.com/p/fotowall                                 *
 *                                                                         *
 *   Copyright (C) 2007-2009 by Enrico Ros <enrico.ros@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "FotoWall.h"
#include "items/VideoProvider.h"
#include "ui_FotoWall.h"
#include "Desk.h"
#include "ExportWizard.h"
#include "XmlRead.h"
#include "XmlSave.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFile>
#include <QImageReader>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include "ModeInfo.h"
#include "ExactSizeDialog.h"

// current location and 'check string' for the tutorial
#define TUTORIAL_URL QUrl("http://fosswire.com/post/2008/09/fotowall-make-wallpaper-collages-from-your-photos/")
#define TUTORIAL_STRING "Peter walks you through how to use Foto"

class FWGraphicsView : public QGraphicsView {
    public:
        FWGraphicsView(Desk * desk, QWidget * parent)
            : QGraphicsView(desk, parent)
            , m_desk(desk)
        {
            // customize widget
            setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            setInteractive(true);
            setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing /*| QPainter::SmoothPixmapTransform */);
            // FIXME: enable the RubberBandDrag. I get rendering artifacts (Enrico)
            setDragMode(QGraphicsView::NoDrag);
            setAcceptDrops(true);
            setFrameStyle(QFrame::NoFrame);

            // don't autofill the view with the Base brush
            QPalette pal;
            pal.setBrush(QPalette::Base, Qt::NoBrush);
            setPalette(pal);

            // can't activate the cache mode by default, since it inhibits dynamical background picture changing
            //setCacheMode(CacheBackground);
        }

    protected:
        void resizeEvent(QResizeEvent * event)
        {
            m_desk->resize(contentsRect().size());
            QGraphicsView::resizeEvent(event);
        }

    private:
        Desk * m_desk;
};

FotoWall::FotoWall(QWidget * parent)
    : QWidget(parent)
    , ui(new Ui::FotoWall())
    , m_view(0)
    , m_desk(0)
    , m_aHelpTutorial(0)
    , m_aHelpSupport(0)
{
    // setup widget
    QRect geom = QApplication::desktop()->availableGeometry();
    resize(2 * geom.width() / 3, 2 * geom.height() / 3);
    setWindowTitle(qApp->applicationName() + " " + qApp->applicationVersion());
    setWindowIcon(QIcon(":/data/fotowall.png"));
#if 0 //QT_VERSION >= 0x040500
    // this produces cool results, but it's premature for something this heavy
    setAttribute(Qt::WA_TranslucentBackground, true);
#endif

    // create our custom desk
    m_desk = new Desk(this);

    // init ui
    ui->setupUi(this);

    // add the graphicsview
    m_view = new FWGraphicsView(m_desk, ui->centralWidget);
    QVBoxLayout * lay = new QVBoxLayout(ui->centralWidget);
    lay->setAlignment(Qt::AlignHCenter);
    lay->setSpacing(0);
    lay->setMargin(0);
    lay->addWidget(m_view);
    m_view->setFocus();

    // attach menus
    ui->arrangeButton->setMenu(createArrangeMenu());
    ui->backButton->setMenu(createBackgroundMenu());
    ui->decoButton->setMenu(createDecorationMenu());
    ui->howtoButton->setMenu(createHelpMenu());

    // react to VideoProvider
    ui->aAddVideo->setVisible(VideoProvider::instance()->inputCount() > 0);
    connect(VideoProvider::instance(), SIGNAL(inputCountChanged(int)), this, SLOT(slotVerifyVideoInputs(int)));

    // create misc actions
    createMiscActions();

    // set the startup project mode
    on_projectType_currentIndexChanged(0);
    m_modeInfo.setDeskDpi(m_view->logicalDpiX(), m_view->logicalDpiY());
    m_modeInfo.setPrintDpi(300);

    // check stuff on the net
    checkForTutorial();
    checkForSupport();
}

FotoWall::~FotoWall()
{
    // dump current layout
    saveXml(QDir::tempPath() + QDir::separator() + "autosave.fotowall");

    // delete everything
    delete m_view;
    delete m_desk;
    delete ui;
}

void FotoWall::setModeInfo(ModeInfo modeInfo)
{
    m_modeInfo = modeInfo;
    m_modeInfo.setDeskDpi(m_view->logicalDpiX(), m_view->logicalDpiY());
}

ModeInfo FotoWall::getModeInfo()
{
    return m_modeInfo;
}

void FotoWall::restoreMode(int mode)
{
    on_projectType_currentIndexChanged(mode);
}

void FotoWall::loadXml(const QString & filePath)
{
    if (filePath.isNull())
        return;
    XmlRead *xmlRead = 0;
    try {
        xmlRead = new XmlRead(filePath);
    } catch (...) {
        // If loading failed
        return;
    }
    xmlRead->readProject(this);
    xmlRead->readDesk(m_desk);
    xmlRead->readContent(m_desk);
    delete xmlRead;
}

void FotoWall::saveXml(const QString & filePath) const
{
    XmlSave *xmlSave = 0;
    try {
        xmlSave = new XmlSave(filePath);
    } catch (...) {
        //if saving failled
        return;
    }
    xmlSave->saveProject(m_desk->projectMode(), m_modeInfo);
    xmlSave->saveDesk(m_desk);
    xmlSave->saveContent(m_desk);
    delete xmlSave;
}

void FotoWall::showIntroduction()
{
    m_desk->showIntroduction();
}

QMenu * FotoWall::createArrangeMenu()
{
    QMenu * menu = new QMenu();

    QAction * aForceField = new QAction(tr("Enable force field"), menu);
    aForceField->setCheckable(true);
    aForceField->setChecked(m_desk->forceFieldEnabled());
    connect(aForceField, SIGNAL(toggled(bool)), this, SLOT(slotArrangeForceField(bool)));
    menu->addAction(aForceField);

    QAction * aNP = new QAction(tr("Auto-arrange new pictures"), menu);
    aNP->setCheckable(true);
    aNP->setChecked(false);
    //connect(aNP, SIGNAL(toggled(bool)), this, SLOT(slotArrangeNew(bool)));
    menu->addAction(aNP);

    menu->addSeparator()->setText(tr("Rearrange"));

    QAction * aAU = new QAction(tr("Uniform"), menu);
    aAU->setEnabled(false);
    //connect(aAU, SIGNAL(triggered()), this, SLOT(slotArrangeUniform()));
    menu->addAction(aAU);

    QAction * aAS = new QAction(tr("Shaped"), menu);
    aAS->setEnabled(false);
    //connect(aAS, SIGNAL(triggered()), this, SLOT(slotArrangeShape()));
    menu->addAction(aAS);

    QAction * aAC = new QAction(tr("Collage"), menu);
    aAC->setEnabled(false);
    //connect(aAC, SIGNAL(triggered()), this, SLOT(slotArrangeCollage()));
    menu->addAction(aAC);

    return menu;
}

QMenu * FotoWall::createBackgroundMenu()
{
    QMenu * menu = new QMenu();

    QAction * aGradient = new QAction(tr("Gradient"), menu);
    aGradient->setCheckable(true);
    aGradient->setChecked(m_desk->backGradientEnabled());
    connect(aGradient, SIGNAL(toggled(bool)), this, SLOT(slotBackGradient(bool)));
    menu->addAction(aGradient);

    return menu;
}

QMenu * FotoWall::createDecorationMenu()
{
    QMenu * menu = new QMenu();

    QAction * aTop = new QAction(tr("Top bar"), menu);
    aTop->setCheckable(true);
    aTop->setChecked(m_desk->topBarEnabled());
    connect(aTop, SIGNAL(toggled(bool)), this, SLOT(slotDecoTopBar(bool)));
    menu->addAction(aTop);

    QAction * aBottom = new QAction(tr("Bottom bar"), menu);
    aBottom->setCheckable(true);
    aBottom->setChecked(m_desk->bottomBarEnabled());
    connect(aBottom, SIGNAL(toggled(bool)), this, SLOT(slotDecoBottomBar(bool)));
    menu->addAction(aBottom);

    menu->addSeparator();

    QAction * aSetTitle = new QAction(tr("Set title..."), menu);
    aSetTitle->setShortcut(tr("CTRL+T"));
    connect(aSetTitle, SIGNAL(triggered()), this, SLOT(slotDecoSetTitle()));
    menu->addAction(aSetTitle);

    QAction * aClearTitle = new QAction(tr("Clear title"), menu);
    connect(aClearTitle, SIGNAL(triggered()), this, SLOT(slotDecoClearTitle()));
    menu->addAction(aClearTitle);

    return menu;
}

QMenu * FotoWall::createHelpMenu()
{
    QMenu * menu = new QMenu();

    QAction * aIntroduction = new QAction(tr("Introduction"), menu);
    connect(aIntroduction, SIGNAL(triggered()), this, SLOT(slotHelpIntroduction()));
    menu->addAction(aIntroduction);

    m_aHelpTutorial = new QAction(tr("Tutorial Video"), menu);
    connect(m_aHelpTutorial, SIGNAL(triggered()), this, SLOT(slotHelpTutorial()));
    menu->addAction(m_aHelpTutorial);

    m_aHelpSupport = new QAction("", menu);
    connect(m_aHelpSupport, SIGNAL(triggered()), this, SLOT(slotHelpSupport()));
    menu->addAction(m_aHelpSupport);

    return menu;
}

void FotoWall::createMiscActions()
{
    // select all
    QAction * aSA = new QAction(tr("Select all"), this);
    aSA->setShortcut(tr("CTRL+A"));
    connect(aSA, SIGNAL(triggered()), this, SLOT(slotActionSelectAll()));
    addAction(aSA);
}

void FotoWall::checkForTutorial()
{
    // hide the tutorial link
    m_aHelpTutorial->setVisible(false);

    // try to get the tutorial page (note, multiple QNAMs will be deleted on app closure)
    QNetworkAccessManager * manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slotVerifyTutorial(QNetworkReply*)));
    manager->get(QNetworkRequest(TUTORIAL_URL));
}

void FotoWall::checkForSupport()
{
    // hide the support link
    m_aHelpSupport->setVisible(false);

    // check the Open Collaboration Services knowledgebase for FotoWall
    // TODO
    QTimer::singleShot(2000, this, SLOT(slotVerifySupport()));
}

void FotoWall::setNormalProject()
{
    m_modeInfo.setRealSizeInches(-1,-1); // Unset the size (for the saving function)
    static bool skipFirstMaximizeHack = true;
    m_view->setMinimumSize(m_view->minimumSizeHint());
    m_view->setMaximumSize(QSize(16777215, 16777215));
    if (skipFirstMaximizeHack)
        skipFirstMaximizeHack = false;
    else
        showMaximized();
    ui->exportButton->setText(tr("export"));
    m_desk->setProjectMode(Desk::ModeNormal);
    ui->projectType->setCurrentIndex(0);
}

void FotoWall::setCDProject()
{
    // A CD cover is a 4.75x4.715 inches square.
    m_modeInfo.setRealSizeInches(4.75, 4.75);
    m_modeInfo.setLandscape(false);
    m_view->setFixedSize(m_modeInfo.deskPixelSize());
    showNormal();
    ui->exportButton->setText(tr("print"));
    m_desk->setProjectMode(Desk::ModeCD);
    ui->projectType->setCurrentIndex(1);
}

void FotoWall::setDVDProject()
{
    m_modeInfo.setRealSizeInches((float)10.83, (float)7.2);
    m_modeInfo.setLandscape(true);
    m_view->setFixedSize(m_modeInfo.deskPixelSize());
    showNormal();
    ui->exportButton->setText(tr("print"));
    m_desk->setProjectMode(Desk::ModeDVD);
    ui->projectType->setCurrentIndex(2);
}

void FotoWall::setExactSizeProject()
{
    // Exact size mode
    if(m_modeInfo.realSize().isEmpty()) {
        ExactSizeDialog sizeDialog;
        QPointF screenDpi = m_modeInfo.deskDpi();
        if (screenDpi.x() == screenDpi.y())
            sizeDialog.ui.screenDpi->setValue(screenDpi.x());
        else
            sizeDialog.ui.screenDpi->setSpecialValueText(QString("%1, %2").arg(screenDpi.x()).arg(screenDpi.y()));
        if(sizeDialog.exec() != QDialog::Accepted) {
            return;
        }
        float w = sizeDialog.ui.widthSpinBox->value();
        float h = sizeDialog.ui.heightSpinBox->value();
        int printDpi = sizeDialog.ui.printDpi->value();
        bool landscape = sizeDialog.ui.landscapeCheckBox->isChecked();
        m_modeInfo.setLandscape(landscape);
        m_modeInfo.setPrintDpi(printDpi);
        if(sizeDialog.ui.unityComboBox->currentIndex() == 0)
            m_modeInfo.setRealSizeCm(w, h);
        else
            m_modeInfo.setRealSizeInches(w, h);
    }
    m_view->setFixedSize(m_modeInfo.deskPixelSize());
    showNormal();
    ui->exportButton->setText(tr("print"));
    m_desk->setProjectMode(Desk::ModeExactSize);
    ui->projectType->setCurrentIndex(3);
}

void FotoWall::on_projectType_currentIndexChanged(int index)
{
    m_modeInfo.setRealSizeInches(-1,-1); // Unset the size (so if it is a mode that require
                                        // asking size, it will be asked !
    switch (index) {
        case 0: //Normal project
            setNormalProject();
            break;

        case 1: // CD cover
            setCDProject();
            break;

        case 2: //DVD cover
            setDVDProject();
            break;

        case 3: //Exact Size
            setExactSizeProject();
            break;
    }
}

void FotoWall::on_aAddFlickr_toggled(bool on)
{
    m_desk->setWebContentSelectorVisible(on);
}

void FotoWall::on_aAddPicture_triggered()
{
    // build the extensions list
    QString extensions;
    foreach (const QByteArray & format, QImageReader::supportedImageFormats())
        extensions += "*." + format + " ";

    // show the files dialog
    QStringList fileNames = QFileDialog::getOpenFileNames(ui->centralWidget, tr("Select one or more pictures to add"), QString(), tr("Images (%1)").arg(extensions));
    if (!fileNames.isEmpty())
        m_desk->addPictures(fileNames);
}

void FotoWall::on_aAddText_triggered()
{
    m_desk->addTextContent();
}

void FotoWall::on_aAddVideo_triggered()
{
    m_desk->addVideoContent(0);
}

void FotoWall::on_loadButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select FotoWall file"), QString(), tr("FotoWall (*.fotowall)"));
    loadXml(fileName);
}

void FotoWall::on_saveButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Select FotoWall file"), QString(), "FotoWall (*.fotowall)");
    if (fileName.isNull())
        return;
    if (!fileName.endsWith(".fotowall", Qt::CaseInsensitive))
        fileName += ".fotowall";
    saveXml(fileName);
}

void FotoWall::on_exportButton_clicked()
{
    // show the Export Wizard on normal mode
    if (m_desk->projectMode() == Desk::ModeNormal) {
        ExportWizard(m_desk).exec();
        return;
    }

    // print on other modes
    m_desk->printAsImage(m_modeInfo.printDpi(), m_modeInfo.printPixelSize(), m_modeInfo.landscape());
}

void FotoWall::on_quitButton_clicked()
{
    QCoreApplication::quit();
}

void FotoWall::slotActionSelectAll()
{
    m_desk->selectAllContent();
}

void FotoWall::slotArrangeForceField(bool checked)
{
    m_desk->setForceFieldEnabled(checked);
}

void FotoWall::slotBackGradient(bool checked)
{
    m_desk->setBackGradientEnabled(checked);
}

void FotoWall::slotDecoTopBar(bool checked)
{
    m_desk->setTopBarEnabled(checked);
}

void FotoWall::slotDecoBottomBar(bool checked)
{
    m_desk->setBottomBarEnabled(checked);
}

void FotoWall::slotDecoSetTitle()
{
    // set a dummy title, if none
    if (m_desk->titleText().isEmpty())
        m_desk->setTitleText("...");

    // change title dialog
    bool ok = false;
    QString title = QInputDialog::getText(0, tr("Title"), tr("Insert the title"), QLineEdit::Normal, m_desk->titleText(), &ok);
    if (ok)
        m_desk->setTitleText(title);
}

void FotoWall::slotDecoClearTitle()
{
    m_desk->setTitleText(QString());
}

void FotoWall::slotHelpIntroduction()
{
    m_desk->showIntroduction();
}

void FotoWall::slotHelpTutorial()
{
    int answer = QMessageBox::question(this, tr("Opening the Web Tutorial"), tr("The Tutorial is provided on Fosswire by Peter Upfold.\nDo you want to open the web page?"), QMessageBox::Yes, QMessageBox::No);
    if (answer == QMessageBox::Yes)
        QDesktopServices::openUrl(TUTORIAL_URL);
}

void FotoWall::slotHelpSupport()
{
}

void FotoWall::slotVerifyTutorial(QNetworkReply * reply)
{
    if (reply->error() != QNetworkReply::NoError)
        return;

    QString htmlCode = reply->readAll();
    bool tutorialValid = htmlCode.contains(TUTORIAL_STRING, Qt::CaseInsensitive);
    m_aHelpTutorial->setVisible(tutorialValid);
}

void FotoWall::slotVerifySupport(/*const KnowledgeItemV1List & items*/)
{
    int supportEntries = 0;
    m_aHelpSupport->setVisible(supportEntries > 0);
    m_aHelpSupport->setText(tr("Support (%1)").arg(supportEntries));
/*
    qWarning("FotoWall::slotOcsKbItems: got %d items", items.size());
    foreach (KnowledgeItemV1 * item, items) {
        qWarning() << item->name() << item->description() << item->answer();
    }
*/
}

void FotoWall::slotVerifyVideoInputs(int count)
{
    // maybe blink or something?
    ui->aAddVideo->setVisible(count > 0);
}

