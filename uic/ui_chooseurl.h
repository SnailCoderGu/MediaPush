/********************************************************************************
** Form generated from reading UI file 'chooseurl.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CHOOSEURL_H
#define UI_CHOOSEURL_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ChooseUrlUi
{
public:
    QWidget *layoutWidget;
    QHBoxLayout *hboxLayout;
    QSpacerItem *spacerItem;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QLineEdit *lineEdit;

    void setupUi(QDialog *ChooseUrlUi)
    {
        if (ChooseUrlUi->objectName().isEmpty())
            ChooseUrlUi->setObjectName(QString::fromUtf8("ChooseUrlUi"));
        ChooseUrlUi->setEnabled(true);
        ChooseUrlUi->resize(450, 150);
        QSizePolicy sizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(ChooseUrlUi->sizePolicy().hasHeightForWidth());
        ChooseUrlUi->setSizePolicy(sizePolicy);
        ChooseUrlUi->setStyleSheet(QString::fromUtf8(""));
        layoutWidget = new QWidget(ChooseUrlUi);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        layoutWidget->setGeometry(QRect(20, 100, 381, 33));
        hboxLayout = new QHBoxLayout(layoutWidget);
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        spacerItem = new QSpacerItem(131, 31, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);

        okButton = new QPushButton(layoutWidget);
        okButton->setObjectName(QString::fromUtf8("okButton"));

        hboxLayout->addWidget(okButton);

        cancelButton = new QPushButton(layoutWidget);
        cancelButton->setObjectName(QString::fromUtf8("cancelButton"));

        hboxLayout->addWidget(cancelButton);

        lineEdit = new QLineEdit(ChooseUrlUi);
        lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
        lineEdit->setGeometry(QRect(30, 50, 371, 20));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lineEdit->sizePolicy().hasHeightForWidth());
        lineEdit->setSizePolicy(sizePolicy1);
        lineEdit->setMaximumSize(QSize(800, 400));

        retranslateUi(ChooseUrlUi);
        QObject::connect(okButton, SIGNAL(clicked()), ChooseUrlUi, SLOT(accept()));
        QObject::connect(cancelButton, SIGNAL(clicked()), ChooseUrlUi, SLOT(reject()));

        QMetaObject::connectSlotsByName(ChooseUrlUi);
    } // setupUi

    void retranslateUi(QDialog *ChooseUrlUi)
    {
        ChooseUrlUi->setWindowTitle(QApplication::translate("ChooseUrlUi", "Dialog", nullptr));
        okButton->setText(QApplication::translate("ChooseUrlUi", "OK", nullptr));
        cancelButton->setText(QApplication::translate("ChooseUrlUi", "Cancel", nullptr));
        lineEdit->setPlaceholderText(QApplication::translate("ChooseUrlUi", "rtmp://192.168.109.128:1935/live/test", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ChooseUrlUi: public Ui_ChooseUrlUi {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CHOOSEURL_H
