#pragma once

#include <QDialog>

namespace Ui {
    class AboutDialog;
}

namespace pboman3 {
    class AboutDialog : public QDialog {
    public:
        AboutDialog(QWidget* parent = nullptr);

        ~AboutDialog();

    private:
        Ui::AboutDialog* ui_;
    };
}