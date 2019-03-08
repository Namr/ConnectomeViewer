#include "mrisettings.h"
#include "ui_mrisettings.h"

MriSettings::MriSettings(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MriSettings)
{
    ui->setupUi(this);

    coronal.x = -20;
    coronal.y = -72;
    coronal.z = -127;

    coronal.sx = 72;
    coronal.sy = 1.0f;
    coronal.sz = 258;

    axial.x = -30;
    axial.y = -150;
    axial.z = -126;

    axial.sx = 95;
    axial.sy = 1.0f;
    axial.sz = 257;

    ui->XPos->setValue(coronal.x);
    ui->YPos->setValue(coronal.y);
    ui->ZPos->setValue(coronal.z);
    ui->XScale->setValue(coronal.sx);
    ui->YScale->setValue(coronal.sy);
    ui->ZScale->setValue(coronal.sz);
}

MriSettings::~MriSettings()
{
    delete ui;
}

//coronal
void MriSettings::on_radioButton_clicked()
{
    curSlice = 0;
    ui->XPos->setValue(coronal.x);
    ui->YPos->setValue(coronal.y);
    ui->ZPos->setValue(coronal.z);
    ui->XScale->setValue(coronal.sx);
    ui->YScale->setValue(coronal.sy);
    ui->ZScale->setValue(coronal.sz);
}

//axial
void MriSettings::on_radioButton_2_clicked()
{
    curSlice = 1;
    ui->XPos->setValue(axial.x);
    ui->YPos->setValue(axial.y);
    ui->ZPos->setValue(axial.z);
    ui->XScale->setValue(axial.sx);
    ui->YScale->setValue(axial.sy);
    ui->ZScale->setValue(axial.sz);
}

void MriSettings::on_XPos_valueChanged(int arg1)
{
    if(curSlice == 0)
    {
        coronal.x = arg1;
    }
    else if(curSlice == 1)
    {
        axial.x = arg1;
    }
}

void MriSettings::on_YPos_valueChanged(int arg1)
{
    if(curSlice == 0)
    {
        coronal.y = arg1;
    }
    else if(curSlice == 1)
    {
        axial.y = arg1;
    }
}

void MriSettings::on_ZPos_valueChanged(int arg1)
{
    if(curSlice == 0)
    {
        coronal.z = arg1;
    }
    else if(curSlice == 1)
    {
        axial.z = arg1;
    }
}

void MriSettings::on_XScale_valueChanged(int arg1)
{
    if(curSlice == 0)
    {
        coronal.sx = arg1;
    }
    else if(curSlice == 1)
    {
        axial.sx = arg1;
    }
}

void MriSettings::on_YScale_valueChanged(int arg1)
{
    if(curSlice == 0)
    {
        coronal.sy = arg1;
    }
    else if(curSlice == 1)
    {
        axial.sy = arg1;
    }
}

void MriSettings::on_ZScale_valueChanged(int arg1)
{
    if(curSlice == 0)
    {
        coronal.sz = arg1;
    }
    else if(curSlice == 1)
    {
        axial.sz = arg1;
    }
}
