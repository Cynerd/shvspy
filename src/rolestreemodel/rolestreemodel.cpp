#include "rolestreemodel.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/core/log.h>

#include <shv/broker/aclrole.h>

static const std::string VALUE_METHOD = "value";

RolesTreeModel::RolesTreeModel(QObject * parent)
	: QStandardItemModel(parent)
{
}

bool RolesTreeModel::setData(const QModelIndex & ix, const QVariant & val, int role)
{
	bool ret = true;

	if(role == Qt::CheckStateRole) {
		ret = QStandardItemModel::setData(ix, val, role);

		if (val.toInt() != Qt::PartiallyChecked) {
			checkPartialySubRoles();
		}
	}
	else {
		ret = QStandardItemModel::setData(ix, val, role);
	}
	return ret;
}

void RolesTreeModel::load(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path)
{
	if (rpc_connection == nullptr)
		return;

	int rqid = rpc_connection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(rpc_connection, rqid, this);

	cb->start(this, [this, rpc_connection, acl_etc_roles_node_path](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				emit loadError(tr("Failed to load roles.") + " " + QString::fromStdString(response.error().toString()));
			}
			else{
				if (response.result().isList()){
					QVector<std::string> roles;

					for (const auto &v : response.result().toList()){
						roles.push_back(v.toStdString());
					}

					loadRoles(rpc_connection, acl_etc_roles_node_path, roles);
				}
			}
		}
		else{
			emit loadError(tr("Request timeout expired for method: %1 path: %2").arg("ls").arg(QString::fromStdString(acl_etc_roles_node_path)));
		}
	});

	rpc_connection->callShvMethod(rqid, acl_etc_roles_node_path, shv::chainpack::Rpc::METH_LS);
}

void RolesTreeModel::loadRoles(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path, QVector<std::string> not_loaded_roles)
{
	if (not_loaded_roles.isEmpty()){
		generateTree();
		emit loadFinished();
		return;
	}

	if(rpc_connection == nullptr){
		return;
	}

	int rqid = rpc_connection->nextRequestId();
	std::string role_name = not_loaded_roles.takeFirst();

	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(rpc_connection, rqid, this);
	std::string role_path = acl_etc_roles_node_path + "/" + role_name;

	cb->start(this, [this, rpc_connection, acl_etc_roles_node_path, role_path, role_name, not_loaded_roles](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if (response.isError()) {
				emit loadError(tr("Failed to load role: %1").arg(QString::fromStdString(role_name)) + " " + QString::fromStdString(response.error().toString()));
			}
			else{
				if (response.result().isMap()){
					shv::broker::AclRole r = shv::broker::AclRole::fromRpcValue(response.result());
					m_shvRoles[QString::fromStdString(role_name)] = r.roles;
				}

				loadRoles(rpc_connection, acl_etc_roles_node_path, not_loaded_roles);
			}
		}
		else{
			emit loadError(tr("Request timeout expired for method: %1 path: %2").arg(QString::fromStdString(VALUE_METHOD)).arg(QString::fromStdString(role_path)));
		}
	});

	rpc_connection->callShvMethod(rqid, role_path, VALUE_METHOD);
}

void RolesTreeModel::setSelectedRoles(const std::vector<std::string> &roles)
{
	for (size_t r = 0; r < roles.size(); r++){
		std::string role = roles.at(r);
		QStandardItem *root_item = invisibleRootItem();

		for(int i = 0; i < root_item->rowCount(); i++) {
			QStandardItem *it = root_item->child(i);
			bool check = (role == it->data().toString().toStdString());

			if (check){
				it->setCheckState(Qt::Checked);
			}
		}
	}

	checkPartialySubRoles();
}

std::vector<std::string> RolesTreeModel::selectedRoles()
{
	std::vector<std::string> roles;
	QStandardItem *root_item = invisibleRootItem();

	for(int i = 0; i < root_item->rowCount(); i++) {
		QStandardItem *it = root_item->child(i);

		if (it->checkState() == Qt::CheckState::Checked){
			roles.push_back(it->data().toString().toStdString());
		}
	}

	return roles;
}

void RolesTreeModel::generateTree()
{
	clear();
	QStringList sl;
	sl << tr("Role");
	setHorizontalHeaderLabels(sl);

	QStandardItem *parent_item = invisibleRootItem();

	for (const QString &role_name: m_shvRoles.keys()){
		QList<QStandardItem *> row;
		QStandardItem *it = new QStandardItem(role_name);
		it->setData(role_name, NameRole);
		it->setCheckable(true);
		it->setFlags(it->flags() & ~Qt::ItemIsEditable);
		row << it;
		parent_item->appendRow(row);

		std::vector<std::string> sub_roles = m_shvRoles.value(role_name);
		QSet<QString> created_roles{role_name};

		for (size_t i = 0; i < sub_roles.size(); i++){
			generateSubTree(it, QString::fromStdString(sub_roles.at(i)), created_roles);
		}
	}
}

void RolesTreeModel::generateSubTree(QStandardItem *parent_item, const QString &role, QSet<QString> created_roles)
{
	if (created_roles.contains(role)){
		shvWarning() << "generate sub tree cyclic reference";
		return;
	}

	QList<QStandardItem *> row;
	QStandardItem *it = new QStandardItem(role);
	it->setData(role, NameRole);
	it->setFlags(it->flags() & ~Qt::ItemIsEditable);
	row << it;
	parent_item->appendRow(row);

	std::vector<std::string> subRoles = m_shvRoles.value(role);

	for (size_t j = 0; j < subRoles.size(); j++){
		QString sub_role = QString::fromStdString(subRoles.at(j));
		QSet<QString> cr = created_roles;
		cr += role;

		generateSubTree(it, sub_role, cr);
	}
}

QSet<QString> RolesTreeModel::allSubRoles()
{
	QSet<QString> roles;
	QStandardItem *root_item = invisibleRootItem();

	for (int i = 0; i < root_item->rowCount(); i++) {
		QStandardItem *it = root_item->child(i);

		if (it->checkState() == Qt::CheckState::Checked){
			for(int ch = 0; ch < it->rowCount(); ch++) {
				QStandardItem *it1 = it->child(ch);
				roles += flattenRole(it1);
			}
		}
	}

	return roles;
}

QSet<QString> RolesTreeModel::flattenRole(QStandardItem *parent_item)
{
	QSet<QString> ret;

	ret.insert(parent_item->data().toString());

	for (int i = 0; i < parent_item->rowCount(); i++) {
		QStandardItem *it = parent_item->child(i);
		QString role_name = it->data().toString();

		if(!ret.contains(role_name)) {
			ret += flattenRole(it);
		}
	}

	return ret;
}

void RolesTreeModel::checkPartialySubRoles()
{
	QSet<QString> selected_sub_roles = allSubRoles();
	QStandardItem *root_item = invisibleRootItem();

	for(int i = 0; i < root_item->rowCount(); i++) {
		QStandardItem *it = root_item->child(i);

		if (it->checkState() == Qt::CheckState::PartiallyChecked){
			if (!selected_sub_roles.contains(it->data().toString())){
				it->setCheckState(Qt::CheckState::Unchecked);
			}
		}
		else{
			if (selected_sub_roles.contains(it->data().toString())){
				it->setCheckState(Qt::CheckState::PartiallyChecked);
			}
		}
	}
}
