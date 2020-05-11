#ifndef THEAPP_H
#define THEAPP_H

#include <shv/core/utils/crypt.h>

#include <QApplication>
#include <QDateTime>

class ServerTreeModel;
class AttributesModel;
class RpcNotificationsModel;
class AppCliOptions;
class QSettings;

namespace shv { namespace visu { class ErrorLogModel; } }

class TheApp : public QApplication
{
	Q_OBJECT
private:
	typedef QApplication Super;
public:
	class CallParam {
	public:
		QDateTime time;
		QString param;
	};

	using ParamMap = QMap<QString, QMap<QString, QVector<CallParam>>>;
	TheApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~TheApp() Q_DECL_OVERRIDE;

	static TheApp* instance() {return qobject_cast<TheApp*>(Super::instance());}
	AppCliOptions* cliOptions() {return m_cliOptions;}

	ServerTreeModel* serverTreeModel() {return m_serverTreeModel;}
	AttributesModel* attributesModel() {return m_attributesModel;}
	RpcNotificationsModel* rpcNotificationsModel() {return m_rpcNotificationsModel;}
	shv::visu::ErrorLogModel* errorLogModel() {return m_errorLogModel;}
	const shv::core::utils::Crypt& crypt() {return m_crypt;}
	void addLastUsedParam(const QString &shv_path, const QString &method, const QString &cpon);
	const ParamMap &lastUsedParams() const;

	void loadSettings(QSettings &settings);
	void saveSettings(QSettings &settings);

private:
	ServerTreeModel *m_serverTreeModel = nullptr;
	AttributesModel *m_attributesModel = nullptr;
	RpcNotificationsModel *m_rpcNotificationsModel = nullptr;
	shv::visu::ErrorLogModel *m_errorLogModel = nullptr;
	AppCliOptions* m_cliOptions = nullptr;
	shv::core::utils::Crypt m_crypt;
	ParamMap m_lastUsedParams;
};

#endif // THEAPP_H
