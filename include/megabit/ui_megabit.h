/********************************************************************************
** Form generated from reading UI file 'megabit.ui'
**
** Created by: Qt User Interface Compiler version 5.7.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MEGABIT_H
#define UI_MEGABIT_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Megabit
{
public:
    QAction *actionAbout_Megabit;
    QAction *actionQuit;
    QWidget *centralWidget;
    QGridLayout *gridLayout;
    QTabWidget *transactions;
    QWidget *tab;
    QVBoxLayout *verticalLayout;
    QTableWidget *accountsTable;
    QPushButton *addAccount;
    QWidget *tab_2;
    QWidget *formLayoutWidget;
    QFormLayout *formLayout;
    QLabel *amountBTCLabel;
    QLineEdit *amountBTCLineEdit;
    QLabel *recipientBitcoinAddressLabel;
    QLineEdit *recipientBitcoinAddressLineEdit;
    QLabel *accountToDebitLabel;
    QLineEdit *accountToDebitLineEdit;
    QLabel *transactionFeesLabel;
    QLineEdit *transactionFeesLineEdit;
    QLabel *totalSpentLabel;
    QLineEdit *totalSpentLineEdit;
    QWidget *tab_3;
    QWidget *tab_4;
    QMenuBar *menuBar;
    QMenu *menuFile;
    QMenu *menuSettings;
    QMenu *menuHelp;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *Megabit)
    {
        if (Megabit->objectName().isEmpty())
            Megabit->setObjectName(QStringLiteral("Megabit"));
        Megabit->resize(947, 873);
        Megabit->setAutoFillBackground(false);
        actionAbout_Megabit = new QAction(Megabit);
        actionAbout_Megabit->setObjectName(QStringLiteral("actionAbout_Megabit"));
        actionQuit = new QAction(Megabit);
        actionQuit->setObjectName(QStringLiteral("actionQuit"));
        centralWidget = new QWidget(Megabit);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        gridLayout = new QGridLayout(centralWidget);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        transactions = new QTabWidget(centralWidget);
        transactions->setObjectName(QStringLiteral("transactions"));
        transactions->setMinimumSize(QSize(0, 0));
        tab = new QWidget();
        tab->setObjectName(QStringLiteral("tab"));
        verticalLayout = new QVBoxLayout(tab);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        accountsTable = new QTableWidget(tab);
        accountsTable->setObjectName(QStringLiteral("accountsTable"));

        verticalLayout->addWidget(accountsTable);

        addAccount = new QPushButton(tab);
        addAccount->setObjectName(QStringLiteral("addAccount"));
        addAccount->setEnabled(false);
        addAccount->setCheckable(false);

        verticalLayout->addWidget(addAccount);

        transactions->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QStringLiteral("tab_2"));
        formLayoutWidget = new QWidget(tab_2);
        formLayoutWidget->setObjectName(QStringLiteral("formLayoutWidget"));
        formLayoutWidget->setGeometry(QRect(9, 19, 511, 331));
        formLayout = new QFormLayout(formLayoutWidget);
        formLayout->setSpacing(6);
        formLayout->setContentsMargins(11, 11, 11, 11);
        formLayout->setObjectName(QStringLiteral("formLayout"));
        formLayout->setContentsMargins(0, 0, 0, 0);
        amountBTCLabel = new QLabel(formLayoutWidget);
        amountBTCLabel->setObjectName(QStringLiteral("amountBTCLabel"));

        formLayout->setWidget(0, QFormLayout::LabelRole, amountBTCLabel);

        amountBTCLineEdit = new QLineEdit(formLayoutWidget);
        amountBTCLineEdit->setObjectName(QStringLiteral("amountBTCLineEdit"));

        formLayout->setWidget(0, QFormLayout::FieldRole, amountBTCLineEdit);

        recipientBitcoinAddressLabel = new QLabel(formLayoutWidget);
        recipientBitcoinAddressLabel->setObjectName(QStringLiteral("recipientBitcoinAddressLabel"));

        formLayout->setWidget(1, QFormLayout::LabelRole, recipientBitcoinAddressLabel);

        recipientBitcoinAddressLineEdit = new QLineEdit(formLayoutWidget);
        recipientBitcoinAddressLineEdit->setObjectName(QStringLiteral("recipientBitcoinAddressLineEdit"));

        formLayout->setWidget(1, QFormLayout::FieldRole, recipientBitcoinAddressLineEdit);

        accountToDebitLabel = new QLabel(formLayoutWidget);
        accountToDebitLabel->setObjectName(QStringLiteral("accountToDebitLabel"));

        formLayout->setWidget(2, QFormLayout::LabelRole, accountToDebitLabel);

        accountToDebitLineEdit = new QLineEdit(formLayoutWidget);
        accountToDebitLineEdit->setObjectName(QStringLiteral("accountToDebitLineEdit"));

        formLayout->setWidget(2, QFormLayout::FieldRole, accountToDebitLineEdit);

        transactionFeesLabel = new QLabel(formLayoutWidget);
        transactionFeesLabel->setObjectName(QStringLiteral("transactionFeesLabel"));

        formLayout->setWidget(3, QFormLayout::LabelRole, transactionFeesLabel);

        transactionFeesLineEdit = new QLineEdit(formLayoutWidget);
        transactionFeesLineEdit->setObjectName(QStringLiteral("transactionFeesLineEdit"));

        formLayout->setWidget(3, QFormLayout::FieldRole, transactionFeesLineEdit);

        totalSpentLabel = new QLabel(formLayoutWidget);
        totalSpentLabel->setObjectName(QStringLiteral("totalSpentLabel"));

        formLayout->setWidget(4, QFormLayout::LabelRole, totalSpentLabel);

        totalSpentLineEdit = new QLineEdit(formLayoutWidget);
        totalSpentLineEdit->setObjectName(QStringLiteral("totalSpentLineEdit"));

        formLayout->setWidget(4, QFormLayout::FieldRole, totalSpentLineEdit);

        transactions->addTab(tab_2, QString());
        tab_3 = new QWidget();
        tab_3->setObjectName(QStringLiteral("tab_3"));
        transactions->addTab(tab_3, QString());
        tab_4 = new QWidget();
        tab_4->setObjectName(QStringLiteral("tab_4"));
        transactions->addTab(tab_4, QString());

        gridLayout->addWidget(transactions, 0, 0, 1, 1);

        Megabit->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(Megabit);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 947, 19));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName(QStringLiteral("menuFile"));
        menuSettings = new QMenu(menuBar);
        menuSettings->setObjectName(QStringLiteral("menuSettings"));
        menuHelp = new QMenu(menuBar);
        menuHelp->setObjectName(QStringLiteral("menuHelp"));
        Megabit->setMenuBar(menuBar);
        mainToolBar = new QToolBar(Megabit);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        Megabit->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(Megabit);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        Megabit->setStatusBar(statusBar);

        menuBar->addAction(menuFile->menuAction());
        menuBar->addAction(menuSettings->menuAction());
        menuBar->addAction(menuHelp->menuAction());
        menuFile->addAction(actionQuit);
        menuHelp->addAction(actionAbout_Megabit);

        retranslateUi(Megabit);
        QObject::connect(actionQuit, SIGNAL(triggered()), Megabit, SLOT(close()));

        transactions->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(Megabit);
    } // setupUi

    void retranslateUi(QMainWindow *Megabit)
    {
        Megabit->setWindowTitle(QApplication::translate("Megabit", "Megabit", Q_NULLPTR));
        actionAbout_Megabit->setText(QApplication::translate("Megabit", "About Megabit", Q_NULLPTR));
        actionQuit->setText(QApplication::translate("Megabit", "&Quit", Q_NULLPTR));
        actionQuit->setShortcut(QApplication::translate("Megabit", "Ctrl+Q", Q_NULLPTR));
        addAccount->setText(QApplication::translate("Megabit", "Add Account", Q_NULLPTR));
        transactions->setTabText(transactions->indexOf(tab), QApplication::translate("Megabit", "Accounts", Q_NULLPTR));
        amountBTCLabel->setText(QApplication::translate("Megabit", "Amount BTC", Q_NULLPTR));
        recipientBitcoinAddressLabel->setText(QApplication::translate("Megabit", "Recipient Bitcoin Address", Q_NULLPTR));
        accountToDebitLabel->setText(QApplication::translate("Megabit", "Account to Debit", Q_NULLPTR));
        transactionFeesLabel->setText(QApplication::translate("Megabit", "Transaction Fees", Q_NULLPTR));
        totalSpentLabel->setText(QApplication::translate("Megabit", "Total Spent", Q_NULLPTR));
        transactions->setTabText(transactions->indexOf(tab_2), QApplication::translate("Megabit", "Send", Q_NULLPTR));
        transactions->setTabText(transactions->indexOf(tab_3), QApplication::translate("Megabit", "Receive", Q_NULLPTR));
        transactions->setTabText(transactions->indexOf(tab_4), QApplication::translate("Megabit", "Transactions", Q_NULLPTR));
        menuFile->setTitle(QApplication::translate("Megabit", "File", Q_NULLPTR));
        menuSettings->setTitle(QApplication::translate("Megabit", "Settings", Q_NULLPTR));
        menuHelp->setTitle(QApplication::translate("Megabit", "Help", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class Megabit: public Ui_Megabit {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MEGABIT_H
