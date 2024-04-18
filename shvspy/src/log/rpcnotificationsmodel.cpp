#include "rpcnotificationsmodel.h"

#include <shv/chainpack/rpcmessage.h>

#include <QDateTime>

namespace cp = shv::chainpack;

RpcNotificationsModel::RpcNotificationsModel(QObject *parent)
	: Super(parent)
{
}

QVariant RpcNotificationsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		switch (section) {
		case ColTimeStamp: return tr("Received");
		case ColBroker: return tr("Connection");
		case ColShvPath: return tr("Source");
		case ColMethod: return tr("Method");
		case ColParams: return tr("Params");
		default: break;
		}
	}
	return QVariant();
}

void RpcNotificationsModel::addLogRow(const std::string &broker_name, const shv::chainpack::RpcMessage &msg)
{
	if(msg.isSignal()) {
		cp::RpcSignal ntf(msg);
		shv::visu::LogTableModelRow rw;
		rw.resize(ColCnt);
		rw[ColTimeStamp] = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
		rw[ColBroker] = QString::fromStdString(broker_name);
		rw[ColShvPath] = QString::fromStdString(ntf.shvPath().toString());
		rw[ColMethod] = QString::fromStdString(ntf.method().toString());
		rw[ColParams] = QString::fromStdString(ntf.params().toCpon());
		Super::addLogRow(rw);
	}
}
