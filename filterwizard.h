#ifndef FILTERWIZARD_H
#define FILTERWIZARD_H

#include <QWizard>

class FilterWizard : public QWizard
{
    Q_OBJECT

public:
    enum { Page_General, Page_SerifFamily, Page_SerifStyle, Page_SansFamily, Page_SansStyle, Page_Finish };

    FilterWizard(QWidget *parent = 0);
    void accept() Q_DECL_OVERRIDE;
};

class QCheckBox;
class QPushButton;
class QLabel;
class QGridLayout;

class GeneralPage : public QWizardPage
{
    Q_OBJECT

public:
    GeneralPage(QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;

private:
    QPushButton* serifButton;
    QPushButton* sansButton;
    QPushButton* scriptButton;
    QPushButton* displayButton;
    QPushButton* symbolicButton;

    QCheckBox* extSerifBox;
    QCheckBox* extSansBox;

    void addGeneralBloc(QPushButton** button, QCheckBox** box, int width, int height, const QString& blockName, const QString& extBlocName);
};


class SerifFamilyPage : public QWizardPage
{
    Q_OBJECT

public:
    SerifFamilyPage(QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;

private:
    QPushButton* oldstyle;
    QPushButton* transitional;
    QPushButton* modern;
    QPushButton* slab;

    void addGeneralBloc(QPushButton** button, int width, int height, const QString& blockName);
};

class SerifStylePage : public QWizardPage
{
    Q_OBJECT

public:
    SerifStylePage(QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;

private:
    QPushButton* cove;
    QPushButton* square;
    QPushButton* bone;
    QPushButton* asymmetric;
    QPushButton* triangle;

    void addGeneralBloc(QPushButton** button, int width, int height, const QString& blockName);
};

class SansFamilyPage : public QWizardPage
{
    Q_OBJECT

public:
    SansFamilyPage(QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;

private:
    QPushButton* grotesque;
    QPushButton* geometric;
    QPushButton* humanist;

    void addGeneralBloc(QPushButton** button, int width, int height, const QString& blockName);
};

class SansStylePage : public QWizardPage
{
    Q_OBJECT

public:
    SansStylePage(QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;

private:
    QPushButton* normal;
    QPushButton* rounded;
    QPushButton* flarred;

    void addGeneralBloc(QPushButton** button, int width, int height, const QString& blockName);
};

class FinishPage : public QWizardPage
{
    Q_OBJECT

public:
    FinishPage(QWidget *parent = 0);
    int nextId() const Q_DECL_OVERRIDE;

private:
    QCheckBox* cyrillicBox;
    QCheckBox* monospacedBox;
};

#endif // FILTERWIZARD_H
