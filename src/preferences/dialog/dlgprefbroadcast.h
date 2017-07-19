#ifndef DLGPREFBROADCAST_H
#define DLGPREFBROADCAST_H

#include <QModelIndex>
#include <QWidget>

#include "preferences/dialog/ui_dlgprefbroadcastdlg.h"
#include "control/controlobject.h"
#include "preferences/usersettings.h"
#include "broadcast/defs_broadcast.h"
#include "preferences/dlgpreferencepage.h"
#include "preferences/broadcastsettings.h"

class ControlProxy;

class DlgPrefBroadcast : public DlgPreferencePage, public Ui::DlgPrefBroadcastDlg  {
    Q_OBJECT
  public:
    DlgPrefBroadcast(QWidget *parent,
                     UserSettingsPointer _config,
                     BroadcastSettingsPointer pBroadcastSettings);
    virtual ~DlgPrefBroadcast();

  public slots:
    /** Apply changes to widget */
    void slotApply();
    void slotUpdate();
    void slotResetToDefaults();
    void broadcastEnabledChanged(double value);
    void checkBoxEnableReconnectChanged(int value);
    void checkBoxLimitReconnectsChanged(int value);
    void enableCustomMetadataChanged(int value);
    void btnCreateConnectionClicked(bool enabled);
    void profileListItemSelected(const QModelIndex& index);

  signals:
    void apply(const QString &);

  private slots:
    void formValueChanged();
    void onRemoveButtonClicked(int column, int row);
    void onSectionResized();

  private:
    void getValuesFromProfile(BroadcastProfilePtr profile);
    void setValuesToProfile(BroadcastProfilePtr profile);
    void enableValueSignals(bool enable = true);

    BroadcastSettingsPointer m_pBroadcastSettings;
    ControlProxy* m_pBroadcastEnabled;
    BroadcastProfilePtr m_pProfileListSelection;
    bool m_valuesChanged;
};

#endif
