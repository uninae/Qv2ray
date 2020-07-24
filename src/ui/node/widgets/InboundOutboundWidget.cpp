#include "InboundOutboundWidget.hpp"

#include "base/Qv2rayBase.hpp"
#include "core/CoreUtils.hpp"
#include "core/handler/ConfigHandler.hpp"
#include "ui/common/UIBase.hpp"
#include "ui/editors/w_InboundEditor.hpp"
#include "ui/editors/w_JsonEditor.hpp"
#include "ui/editors/w_OutboundEditor.hpp"

InboundOutboundWidget::InboundOutboundWidget(TagNodeMode mode, std::shared_ptr<NodeDispatcher> _d, QWidget *parent) : QvNodeWidget(_d, parent)
{
    workingMode = mode;
    setupUi(this);
    editBtn->setIcon(QICON_R("edit"));
    editJsonBtn->setIcon(QICON_R("code"));
}

void InboundOutboundWidget::setValue(std::shared_ptr<INBOUND> data)
{
    assert(workingMode == NODE_INBOUND);
    inboundObject = data;
    tagTxt->setText(getTag(*data));
}

void InboundOutboundWidget::setValue(std::shared_ptr<OutboundObjectMeta> data)
{
    assert(workingMode == NODE_OUTBOUND);
    outboundObject = data;
    tagTxt->setText(outboundObject->getTag());
    isExternalOutbound = outboundObject->object.mode == MODE_CONNECTIONID;
    statusLabel->setText(isExternalOutbound ? tr("External Config") : "");
    tagTxt->setEnabled(!isExternalOutbound);
}

void InboundOutboundWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange:
        {
            retranslateUi(this);
            break;
        }
        default: break;
    }
}

void InboundOutboundWidget::on_editBtn_clicked()
{
    switch (workingMode)
    {
        case NODE_INBOUND:
        {
            InboundEditor editor{ *inboundObject, parentWidget() };
            *inboundObject = editor.OpenEditor();
            // Set tag
            const auto newTag = getTag(*inboundObject);
            tagTxt->setText(newTag);
            (*inboundObject)["tag"] = newTag;
            break;
        }
        case NODE_OUTBOUND:
        {
            if (isExternalOutbound)
            {
                if (QvMessageBoxAsk(parentWidget(), tr("Edit Outbound"), editExternalMsg) != QMessageBox::Yes)
                {
                    return;
                }
                const auto externalId = outboundObject->object.connectionId;
                const auto root = ConnectionManager->GetConnectionRoot(externalId);
                if (IsComplexConfig(root))
                {
                    if (QvMessageBoxAsk(parentWidget(), tr("Trying to edit an Complex Config"), editExternalComplexMsg) != QMessageBox::Yes)
                    {
                        return;
                    }
                }
                OUTBOUND outbound{ QJsonIO::GetValue(root, "outbounds", 0).toObject() };
                OutboundEditor editor{ outbound, parentWidget() };
                outbound = editor.OpenEditor();
                //
                ConnectionManager->UpdateConnection(externalId, CONFIGROOT{ QJsonObject{ { "outbounds", QJsonArray{ outbound } } } });
            }
            else
            {
                OutboundEditor editor{ outboundObject->realOutbound, parentWidget() };
                outboundObject->realOutbound = editor.OpenEditor();
                // Set tag
                const auto newTag = getTag(outboundObject->realOutbound);
                tagTxt->setText(newTag);
                outboundObject->realOutbound["tag"] = newTag;
                break;
            }
        }
    }
}

void InboundOutboundWidget::on_editJsonBtn_clicked()
{
    switch (workingMode)
    {
        case NODE_INBOUND:
        {
            JsonEditor editor{ *inboundObject, parentWidget() };
            *inboundObject = INBOUND{ editor.OpenEditor() };
            const auto newTag = getTag(*inboundObject);
            // Set tag
            tagTxt->setText(newTag);
            (*inboundObject)["tag"] = newTag;
            break;
        }
        case NODE_OUTBOUND:
        {
            if (isExternalOutbound)
            {
                if (QvMessageBoxAsk(parentWidget(), tr("Edit Outbound"), editExternalMsg) != QMessageBox::Yes)
                {
                    return;
                }
                const auto externalId = outboundObject->object.connectionId;
                const auto root = ConnectionManager->GetConnectionRoot(externalId);

                OUTBOUND outbound{ QJsonIO::GetValue(root, "outbounds", 0).toObject() };
                JsonEditor editor{ outbound, parentWidget() };
                outbound = OUTBOUND{ editor.OpenEditor() };
                //
                ConnectionManager->UpdateConnection(externalId, CONFIGROOT{ QJsonObject{ { "outbounds", QJsonArray{ outbound } } } });
            }
            else
            {
                // Open Editor
                JsonEditor editor{ outboundObject->realOutbound, parentWidget() };
                outboundObject->realOutbound = OUTBOUND{ editor.OpenEditor() };
                //
                // Set tag (only for local connections)
                const auto newTag = getTag(outboundObject->realOutbound);
                tagTxt->setText(newTag);
                outboundObject->realOutbound["tag"] = newTag;
                break;
            }
        }
    }
}

void InboundOutboundWidget::on_tagTxt_textEdited(const QString &arg1)
{
    switch (workingMode)
    {
        case NODE_INBOUND:
        {
            const auto originalTag = (*inboundObject)["tag"].toString();
            if (originalTag == arg1 || dispatcher->RenameTag<NODE_INBOUND>(originalTag, arg1))
            {
                BLACK(tagTxt);
                (*inboundObject)["tag"] = arg1;
                break;
            }
            RED(tagTxt);
        }
        case NODE_OUTBOUND:
        {
            const auto originalTag = outboundObject->getTag();
            if (originalTag == arg1 || dispatcher->RenameTag<NODE_OUTBOUND>(originalTag, arg1))
            {
                BLACK(tagTxt);
                outboundObject->realOutbound["tag"] = arg1;
                break;
            }
            RED(tagTxt);
        }
    }
}
