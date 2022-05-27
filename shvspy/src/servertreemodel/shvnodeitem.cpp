#include "shvnodeitem.h"
#include "shvbrokernodeitem.h"
#include "servertreemodel.h"
#include "../theapp.h"

#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/core/utils/shvpath.h>

#include <shv/core/assert.h>

#include <QIcon>
#include <QVariant>

namespace cp = shv::chainpack;

cp::MetaMethod::Signature ShvMetaMethod::signature() const
{
	return static_cast<cp::MetaMethod::Signature>(methodAttributes.value("signature").toInt());
}

unsigned ShvMetaMethod::flags() const
{
	return methodAttributes.value("flags").toInt();
}

shv::chainpack::RpcValue ShvMetaMethod::accessGrant() const
{
	return methodAttributes.value("accessGrant");
}

std::string ShvMetaMethod::signatureStr() const
{
	std::string ret;
	switch (signature()) {
	case cp::MetaMethod::Signature::VoidVoid: ret = "void()"; break;
	case cp::MetaMethod::Signature::VoidParam: ret = "void(param)"; break;
	case cp::MetaMethod::Signature::RetVoid: ret = "ret()"; break;
	case cp::MetaMethod::Signature::RetParam: ret = "ret(param)"; break;
	}
	//if(isSignal)
	//	ret = ret + " NTF";
	return ret;
}

std::string ShvMetaMethod::flagsStr() const
{
	std::string ret;
	if(flags() & cp::MetaMethod::Flag::IsSignal)
		ret += (ret.empty()? "": ",") + std::string("SIG");
	if(flags() & cp::MetaMethod::Flag::IsGetter)
		ret += (ret.empty()? "": ",") + std::string("G");
	if(flags() & cp::MetaMethod::Flag::IsSetter)
		ret += (ret.empty()? "": ",") + std::string("S");
	return ret;
}

std::string ShvMetaMethod::accessGrantStr() const
{
	cp::AccessGrant ag = cp::AccessGrant::fromRpcValue(accessGrant());
	if(ag.isRole())
		return ag.role;
	if(ag.isAccessLevel()) {
		std::string ret;
		if(ag.accessLevel <= cp::MetaMethod::AccessLevel::Browse)
			ret = "none";
		else if(ag.accessLevel == cp::MetaMethod::AccessLevel::Browse)
			ret = "bws";
		else if(ag.accessLevel < cp::MetaMethod::AccessLevel::Read)
			ret = "bws+";
		else if(ag.accessLevel == cp::MetaMethod::AccessLevel::Read)
			ret = "rd";
		else if(ag.accessLevel < cp::MetaMethod::AccessLevel::Write)
			ret = "rd+";
		else if(ag.accessLevel == cp::MetaMethod::AccessLevel::Write)
			ret = "wr";
		else if(ag.accessLevel < cp::MetaMethod::AccessLevel::Command)
			ret = "wr+";
		else if(ag.accessLevel == cp::MetaMethod::AccessLevel::Command)
			ret = "cmd";
		else if(ag.accessLevel < cp::MetaMethod::AccessLevel::Config)
			ret = "cmd+";
		else if(ag.accessLevel == cp::MetaMethod::AccessLevel::Config)
			ret = "cfg";
		else if(ag.accessLevel < cp::MetaMethod::AccessLevel::Service)
			ret = "cfg+";
		else if(ag.accessLevel == cp::MetaMethod::AccessLevel::Service)
			ret = "svc";
		else if(ag.accessLevel < cp::MetaMethod::AccessLevel::Admin)
			ret = "svc+";
		else if(ag.accessLevel == cp::MetaMethod::AccessLevel::Admin)
			ret = "su";
		else
			ret = "su+";
		return ret;
	}
	return "N/A";
}

bool ShvMetaMethod::isSignal() const
{
	return flags() & cp::MetaMethod::Flag::IsSignal;
}

ShvNodeItem::ShvNodeItem(ServerTreeModel *m, const std::string &ndid, ShvNodeItem *parent)
	: Super(parent)
	, m_nodeId(ndid)
	, m_treeModelId(m->nextId())
{
	setObjectName(QString::fromStdString(ndid));
	m->m_nodes[m_treeModelId] = this;
}

ShvNodeItem::~ShvNodeItem()
{
	treeModel()->m_nodes.remove(m_treeModelId);
}

ServerTreeModel *ShvNodeItem::treeModel() const
{
	for(QObject *o = this->parent(); o; o = o->parent()) {
		ServerTreeModel *m = qobject_cast<ServerTreeModel*>(o);
		if(m)
			return m;
	}
	SHV_EXCEPTION("ServerTreeModel parent must exist.");
}

QVariant ShvNodeItem::data(int role) const
{
	QVariant ret;
	if(role == Qt::DisplayRole) {
		ret = objectName();
	}
	else if(role == Qt::BackgroundRole) {
		if(objectName() == QStringLiteral(".local"))
			return QColor(QStringLiteral("yellowgreen"));
	}
	else if(role == Qt::DecorationRole) {
		if(isChildrenLoading()) {
			static QIcon ico_reload = QIcon(QStringLiteral(":/shvspy/images/reload"));
			ret = ico_reload;
		}
	}
	return ret;
}

ShvBrokerNodeItem *ShvNodeItem::serverNode() const
{
	for(ShvNodeItem *nd = const_cast<ShvNodeItem*>(this); nd; nd=nd->parentNode()) {
		ShvBrokerNodeItem *bnd = qobject_cast<ShvBrokerNodeItem *>(nd);
		if(bnd)
			return bnd;
	}
	SHV_EXCEPTION("ServerNode parent must exist.");
}

ShvNodeItem *ShvNodeItem::parentNode() const
{
	return qobject_cast<ShvNodeItem*>(parent());
}

ShvNodeItem *ShvNodeItem::childAt(int ix) const
{
	if(ix < 0 || ix >= m_children.count())
		SHV_EXCEPTION("Invalid child index");
	return m_children[ix];
}

void ShvNodeItem::insertChild(int ix, ShvNodeItem *n)
{
	ServerTreeModel *m = treeModel();
	m->beginInsertRows(m->indexFromItem(this), ix, ix);
	n->setParent(this);
	m_children.insert(ix, n);
	m->endInsertRows();
}

void ShvNodeItem::deleteChild(int ix)
{
	ShvNodeItem *ret = childAt(ix);
	if(ret) {
		ServerTreeModel *m = treeModel();
		m->beginRemoveRows(m->indexFromItem(this), ix, ix);
		m_children.remove(ix);
		delete ret;
		m->endRemoveRows();
	}
}

void ShvNodeItem::deleteChildren()
{
	if(childCount() == 0)
		return;
	ServerTreeModel *m = treeModel();
	m->beginRemoveRows(m->indexFromItem(this), 0, childCount() - 1);
	qDeleteAll(m_children);
	m_children.clear();
	m->endRemoveRows();
	emitDataChanged();
}

std::string ShvNodeItem::shvPath() const
{
	std::vector<std::string> lst;
	ShvBrokerNodeItem *srv_nd = serverNode();
	const ShvNodeItem *nd = this;
	while(nd) {
		if(!nd || nd == srv_nd)
			break;
		else
			lst.push_back(nd->nodeId());
		nd = nd->parentNode();
	}
	std::reverse(lst.begin(), lst.end());
	std::string path = shv::core::utils::ShvPath::joinDirs(lst);
	path = shv::core::Utils::joinPath(srv_nd->shvRoot(), path);
	return path;
}

void ShvNodeItem::processRpcMessage(const shv::chainpack::RpcMessage &msg)
{
	if(msg.isResponse()) {
		cp::RpcResponse resp(msg);
		int rqid = resp.requestId().toInt();
		if(rqid == m_loadChildrenRqId) {
			m_loadChildrenRqId = 0;
			m_childrenLoaded = true;

			deleteChildren();
			ServerTreeModel *m = treeModel();
			const shv::chainpack::RpcValue::List lst = resp.result().toList();
			for(const cp::RpcValue &dir_entry : lst) {
				const cp::RpcValue::List &long_dir_entry = dir_entry.toList();
				std::string ndid = long_dir_entry.empty()? dir_entry.toString(): long_dir_entry.value(0).toString();
				ShvNodeItem *nd = new ShvNodeItem(m, ndid);
				if(!long_dir_entry.empty()) {
					cp::RpcValue has_children = long_dir_entry.value(1);
					if(has_children.isBool()) {
						nd->setHasChildren(has_children.toBool());
						if(!has_children.toBool())
							nd->setChildrenLoaded();
					}
				}
				appendChild(nd);
			}
			emitDataChanged();
		}
		else if(rqid == m_loadMethodsRqId) {
			m_loadMethodsRqId = 0;
			m_methodsLoaded = true;

			m_methods.clear();
			cp::RpcValue methods = resp.result();
			for(const cp::RpcValue &method : methods.asList()) {
				if(method.isList()) {
					ShvMetaMethod mm;
					cp::RpcValueGenList lst(method);
					mm.method = lst.value(0).toString();
					mm.methodAttributes["signature"] = lst.value(1).toUInt();
					mm.methodAttributes["flags"] = lst.value(2).toUInt();
					mm.methodAttributes["accessGrant"] = lst.value(3);
					mm.methodAttributes["description"] = lst.value(4);
					cp::RpcValue::Map tags = lst.value(5).asMap();
					mm.methodAttributes.merge(tags);
					m_methods.push_back(mm);
				}
				else if(method.isMap()) {
					ShvMetaMethod mm;
					mm.methodAttributes = method.asMap();
					mm.method = mm.methodAttributes.value("name").asString();
					m_methods.push_back(mm);
				}
			}
			emit methodsLoaded();
		}
		else {
			for (int i = 0; i < m_methods.count(); ++i) {
				ShvMetaMethod &mtd = m_methods[i];
				if(mtd.rpcRequestId == rqid) {
					mtd.rpcRequestId = 0;
					mtd.response = resp;
					emit rpcMethodCallFinished(i);
					break;
				}
			}
		}
	}
}

void ShvNodeItem::emitDataChanged()
{
	ServerTreeModel *m = treeModel();
	QModelIndex ix = m->indexFromItem(this);
	emit m->dataChanged(ix, ix);
}

void ShvNodeItem::loadChildren()
{
	m_childrenLoaded = false;
	ShvBrokerNodeItem *srv_nd = serverNode();
	m_loadChildrenRqId = srv_nd->callNodeRpcMethod(shvPath(), cp::Rpc::METH_LS, cp::RpcValue::List{std::string(), (unsigned)0x7F});
	emitDataChanged();
}

bool ShvNodeItem::checkMethodsLoaded()
{
	if(!isMethodsLoaded() && !isMethodsLoading()) {
		loadMethods();
		return false;
	}
	return true;
}

void ShvNodeItem::loadMethods()
{
	m_methodsLoaded = false;
	ShvBrokerNodeItem *srv_nd = serverNode();
	m_loadMethodsRqId = srv_nd->callNodeRpcMethod(shvPath(), cp::Rpc::METH_DIR, cp::RpcValue::List{std::string(), 127});
	//emitDataChanged();
}

void ShvNodeItem::setMethodParams(int method_ix, const shv::chainpack::RpcValue &params)
{
	if(method_ix < 0 || method_ix >= m_methods.count())
		return;
	ShvMetaMethod &mtd = m_methods[method_ix];
	mtd.params = params;
}

unsigned ShvNodeItem::callMethod(int method_ix, bool throw_exc)
{
	//const QVector<ShvMetaMethod> &mm = m_methods();
	if(method_ix < 0 || method_ix >= m_methods.count())
		return 0;
	ShvMetaMethod &mtd = m_methods[method_ix];
	if(mtd.method.empty() || mtd.isSignal())
		return 0;
	mtd.response = cp::RpcResponse();
	ShvBrokerNodeItem *srv_nd = serverNode();
	mtd.rpcRequestId = srv_nd->callNodeRpcMethod(shvPath(), mtd.method, mtd.params, throw_exc);
	return mtd.rpcRequestId;
}

void ShvNodeItem::reload()
{
	deleteChildren();
	m_methods.clear();
	loadChildren();
	loadMethods();
	emitDataChanged();
}

ShvNodeRootItem::ShvNodeRootItem(ServerTreeModel *parent)
	: Super(parent, std::string(), nullptr)
{
	setParent(parent);
}

