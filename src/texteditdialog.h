#pragma once

#include <QDialog>

namespace shv { namespace chainpack { class RpcValue; }}

namespace Ui {
class TextEditDialog;
}

class TextEditDialog : public QDialog
{
	Q_OBJECT

	using Super = QDialog;
public:
	explicit TextEditDialog(QWidget *parent = nullptr);
	~TextEditDialog() override;

	void setText(const QString &s);
	QString text() const;

	void setReadOnly(bool ro);
protected:
	Ui::TextEditDialog *ui;
};

class CponEditDialog : public TextEditDialog
{
	Q_OBJECT

	using Super = TextEditDialog;
public:
	explicit CponEditDialog(QWidget *parent = nullptr);

	void setValidateContent(bool b);
private slots:
	void onBtCompactCponClicked();
	void onBtFormatCponClicked();
private:
	void validateContentDeferred();
	shv::chainpack::RpcValue validateContent();
private:
	bool m_isValidateContent = false;
	QTimer *m_validateTimer = nullptr;
};
