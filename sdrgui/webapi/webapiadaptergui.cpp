///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2017 Edouard Griffiths, F4EXB.                                  //
//                                                                               //
// Swagger server adapter interface                                              //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#include <QApplication>
#include <QList>

#include "mainwindow.h"
#include "loggerwithfile.h"
#include "device/devicesourceapi.h"
#include "device/devicesinkapi.h"
#include "device/deviceuiset.h"
#include "device/deviceenumerator.h"
#include "dsp/devicesamplesource.h"
#include "dsp/devicesamplesink.h"
#include "dsp/dspengine.h"
#include "plugin/pluginapi.h"
#include "plugin/pluginmanager.h"
#include "channel/channelsinkapi.h"
#include "channel/channelsourceapi.h"

#include "SWGInstanceSummaryResponse.h"
#include "SWGInstanceDevicesResponse.h"
#include "SWGInstanceChannelsResponse.h"
#include "SWGDeviceListItem.h"
#include "SWGAudioDevices.h"
#include "SWGAudioDevicesSelect.h"
#include "SWGErrorResponse.h"

#include "webapiadaptergui.h"

WebAPIAdapterGUI::WebAPIAdapterGUI(MainWindow& mainWindow) :
    m_mainWindow(mainWindow)
{
}

WebAPIAdapterGUI::~WebAPIAdapterGUI()
{
}

int WebAPIAdapterGUI::instanceSummary(
            Swagger::SWGInstanceSummaryResponse& response,
            Swagger::SWGErrorResponse& error __attribute__((unused)))
{

    *response.getVersion() = qApp->applicationVersion();

    Swagger::SWGLoggingInfo *logging = response.getLogging();
    logging->init();
    logging->setDumpToFile(m_mainWindow.m_logger->getUseFileLogger() ? 1 : 0);
    if (logging->getDumpToFile()) {
        m_mainWindow.m_logger->getLogFileName(*logging->getFileName());
        m_mainWindow.m_logger->getFileMinMessageLevelStr(*logging->getFileLevel());
    }
    m_mainWindow.m_logger->getConsoleMinMessageLevelStr(*logging->getConsoleLevel());

    Swagger::SWGDeviceSetList *deviceSetList = response.getDevicesetlist();
    deviceSetList->init();
    deviceSetList->setDevicesetcount((int) m_mainWindow.m_deviceUIs.size());

    std::vector<DeviceUISet*>::const_iterator it = m_mainWindow.m_deviceUIs.begin();

    for (int i = 0; it != m_mainWindow.m_deviceUIs.end(); ++it, i++)
    {
        QList<Swagger::SWGDeviceSet*> *deviceSet = deviceSetList->getDeviceSets();
        deviceSet->append(new Swagger::SWGDeviceSet());
        Swagger::SWGSamplingDevice *samplingDevice = deviceSet->back()->getSamplingDevice();
        samplingDevice->init();
        samplingDevice->setIndex(i);
        samplingDevice->setTx((*it)->m_deviceSinkEngine != 0);

        if ((*it)->m_deviceSinkEngine) // Tx data
        {
            *samplingDevice->getHwType() = (*it)->m_deviceSinkAPI->getHardwareId();
            *samplingDevice->getSerial() = (*it)->m_deviceSinkAPI->getSampleSinkSerial();
            samplingDevice->setSequence((*it)->m_deviceSinkAPI->getSampleSinkSequence());
            samplingDevice->setNbStreams((*it)->m_deviceSinkAPI->getNbItems());
            samplingDevice->setStreamIndex((*it)->m_deviceSinkAPI->getItemIndex());
            (*it)->m_deviceSinkAPI->getDeviceEngineStateStr(*samplingDevice->getState());
            DeviceSampleSink *sampleSink = (*it)->m_deviceSinkEngine->getSink();

            if (sampleSink) {
                samplingDevice->setCenterFrequency(sampleSink->getCenterFrequency());
                samplingDevice->setBandwidth(sampleSink->getSampleRate());
            }

            deviceSet->back()->setChannelcount((*it)->m_deviceSinkAPI->getNbChannels());
            QList<Swagger::SWGChannel*> *channels = deviceSet->back()->getChannels();

            for (int i = 0; i <  deviceSet->back()->getChannelcount(); i++)
            {
                channels->append(new Swagger::SWGChannel);
                ChannelSourceAPI *channel = (*it)->m_deviceSinkAPI->getChanelAPIAt(i);
                channels->back()->setDeltaFrequency(channel->getDeltaFrequency());
                channels->back()->setIndex(channel->getIndexInDeviceSet());
                channels->back()->setUid(channel->getUID());
                channel->getIdentifier(*channels->back()->getId());
                channel->getTitle(*channels->back()->getTitle());
            }
        }

        if ((*it)->m_deviceSourceEngine) // Rx data
        {
            *samplingDevice->getHwType() = (*it)->m_deviceSourceAPI->getHardwareId();
            *samplingDevice->getSerial() = (*it)->m_deviceSourceAPI->getSampleSourceSerial();
            samplingDevice->setSequence((*it)->m_deviceSourceAPI->getSampleSourceSequence());
            samplingDevice->setNbStreams((*it)->m_deviceSourceAPI->getNbItems());
            samplingDevice->setStreamIndex((*it)->m_deviceSourceAPI->getItemIndex());
            (*it)->m_deviceSourceAPI->getDeviceEngineStateStr(*samplingDevice->getState());
            DeviceSampleSource *sampleSource = (*it)->m_deviceSourceEngine->getSource();

            if (sampleSource) {
                samplingDevice->setCenterFrequency(sampleSource->getCenterFrequency());
                samplingDevice->setBandwidth(sampleSource->getSampleRate());
            }

            deviceSet->back()->setChannelcount((*it)->m_deviceSourceAPI->getNbChannels());
            QList<Swagger::SWGChannel*> *channels = deviceSet->back()->getChannels();

            for (int i = 0; i <  deviceSet->back()->getChannelcount(); i++)
            {
                channels->append(new Swagger::SWGChannel);
                ChannelSinkAPI *channel = (*it)->m_deviceSourceAPI->getChanelAPIAt(i);
                channels->back()->setDeltaFrequency(channel->getDeltaFrequency());
                channels->back()->setIndex(channel->getIndexInDeviceSet());
                channels->back()->setUid(channel->getUID());
                channel->getIdentifier(*channels->back()->getId());
                channel->getTitle(*channels->back()->getTitle());
            }
        }
    }

    return 200;
}

int WebAPIAdapterGUI::instanceDevices(
            bool tx,
            Swagger::SWGInstanceDevicesResponse& response,
            Swagger::SWGErrorResponse& error __attribute__((unused)))
{
    int nbSamplingDevices = tx ? DeviceEnumerator::instance()->getNbTxSamplingDevices() : DeviceEnumerator::instance()->getNbRxSamplingDevices();
    response.setDevicecount(nbSamplingDevices);
    QList<Swagger::SWGDeviceListItem*> *devices = response.getDevices();

    for (int i = 0; i < nbSamplingDevices; i++)
    {
        PluginInterface::SamplingDevice samplingDevice = tx ? DeviceEnumerator::instance()->getTxSamplingDevice(i) : DeviceEnumerator::instance()->getRxSamplingDevice(i);
        devices->append(new Swagger::SWGDeviceListItem);
        *devices->back()->getDisplayedName() = samplingDevice.displayedName;
        *devices->back()->getHwType() = samplingDevice.hardwareId;
        *devices->back()->getSerial() = samplingDevice.serial;
        devices->back()->setSequence(samplingDevice.sequence);
        devices->back()->setTx(!samplingDevice.rxElseTx);
        devices->back()->setNbStreams(samplingDevice.deviceNbItems);
        devices->back()->setDeviceSetIndex(samplingDevice.claimed);
        devices->back()->setIndex(i);
    }

    return 200;
}

int WebAPIAdapterGUI::instanceChannels(
            bool tx,
            Swagger::SWGInstanceChannelsResponse& response,
            Swagger::SWGErrorResponse& error __attribute__((unused)))
{
    PluginAPI::ChannelRegistrations *channelRegistrations = tx ? m_mainWindow.m_pluginManager->getTxChannelRegistrations() : m_mainWindow.m_pluginManager->getRxChannelRegistrations();
    int nbChannelDevices = channelRegistrations->size();
    response.setChannelcount(nbChannelDevices);
    QList<Swagger::SWGChannelListItem*> *channels = response.getChannels();

    for (int i = 0; i < nbChannelDevices; i++)
    {
        channels->append(new Swagger::SWGChannelListItem);
        PluginInterface *channelInterface = channelRegistrations->at(i).m_plugin;
        const PluginDescriptor& pluginDescriptor = channelInterface->getPluginDescriptor();
        *channels->back()->getVersion() = pluginDescriptor.version;
        *channels->back()->getName() = pluginDescriptor.displayedName;
        channels->back()->setTx(tx);
        *channels->back()->getIdUri() = channelRegistrations->at(i).m_channelIdURI;
        *channels->back()->getId() = channelRegistrations->at(i).m_channelId;
        channels->back()->setIndex(i);
    }

    return 200;
}

int WebAPIAdapterGUI::instanceLoggingGet(
            Swagger::SWGLoggingInfo& response,
            Swagger::SWGErrorResponse& error __attribute__((unused)))
{
    response.setDumpToFile(m_mainWindow.m_logger->getUseFileLogger() ? 1 : 0);

    if (response.getDumpToFile()) {
        m_mainWindow.m_logger->getLogFileName(*response.getFileName());
        m_mainWindow.m_logger->getFileMinMessageLevelStr(*response.getFileLevel());
    }

    m_mainWindow.m_logger->getConsoleMinMessageLevelStr(*response.getConsoleLevel());

    return 200;
}

int WebAPIAdapterGUI::instanceLoggingPut(
            Swagger::SWGLoggingInfo& response,
            Swagger::SWGErrorResponse& error __attribute__((unused)))
{
    // response input is the query actually
    bool dumpToFile = (response.getDumpToFile() != 0);
    QString* consoleLevel = response.getConsoleLevel();
    QString* fileLevel = response.getFileLevel();
    QString* fileName = response.getFileName();

    // perform actions
    if (consoleLevel) {
        m_mainWindow.m_settings.setConsoleMinLogLevel(getMsgTypeFromString(*consoleLevel));
    }

    if (fileLevel) {
        m_mainWindow.m_settings.setFileMinLogLevel(getMsgTypeFromString(*fileLevel));
    }

    m_mainWindow.m_settings.setUseLogFile(dumpToFile);

    if (fileName) {
        m_mainWindow.m_settings.setLogFileName(*fileName);
    }

    m_mainWindow.setLoggingOpions();

    // build response
    response.init();
    getMsgTypeString(m_mainWindow.m_settings.getConsoleMinLogLevel(), *response.getConsoleLevel());
    response.setDumpToFile(m_mainWindow.m_settings.getUseLogFile() ? 1 : 0);
    getMsgTypeString(m_mainWindow.m_settings.getFileMinLogLevel(), *response.getFileLevel());
    *response.getFileName() = m_mainWindow.m_settings.getLogFileName();

    return 200;
}

int WebAPIAdapterGUI::instanceAudioGet(
            Swagger::SWGAudioDevices& response,
            Swagger::SWGErrorResponse& error __attribute__((unused)))
{
    const QList<QAudioDeviceInfo>& audioInputDevices = m_mainWindow.m_audioDeviceInfo.getInputDevices();
    const QList<QAudioDeviceInfo>& audioOutputDevices = m_mainWindow.m_audioDeviceInfo.getOutputDevices();
    int nbInputDevices = audioInputDevices.size();
    int nbOutputDevices = audioOutputDevices.size();

    response.init();
    response.setNbInputDevices(nbInputDevices);
    response.setInputDeviceSelectedIndex(m_mainWindow.m_audioDeviceInfo.getInputDeviceIndex());
    response.setNbOutputDevices(nbOutputDevices);
    response.setOutputDeviceSelectedIndex(m_mainWindow.m_audioDeviceInfo.getOutputDeviceIndex());
    response.setInputVolume(m_mainWindow.m_audioDeviceInfo.getInputVolume());
    QList<Swagger::SWGAudioDevice*> *inputDevices = response.getInputDevices();
    QList<Swagger::SWGAudioDevice*> *outputDevices = response.getOutputDevices();

    for (int i = 0; i < nbInputDevices; i++)
    {
        inputDevices->append(new Swagger::SWGAudioDevice);
        *inputDevices->back()->getName() = audioInputDevices.at(i).deviceName();
    }

    for (int i = 0; i < nbOutputDevices; i++)
    {
        outputDevices->append(new Swagger::SWGAudioDevice);
        *outputDevices->back()->getName() = audioOutputDevices.at(i).deviceName();
    }

    return 200;
}

int WebAPIAdapterGUI::instanceAudioPatch(
            Swagger::SWGAudioDevicesSelect& response,
            Swagger::SWGErrorResponse& error)
{
    // response input is the query actually
    float inputVolume = response.getInputVolume();
    int inputIndex = response.getInputIndex();
    int outputIndex = response.getOutputIndex();

    const QList<QAudioDeviceInfo>& audioInputDevices = m_mainWindow.m_audioDeviceInfo.getInputDevices();
    const QList<QAudioDeviceInfo>& audioOutputDevices = m_mainWindow.m_audioDeviceInfo.getOutputDevices();
    int nbInputDevices = audioInputDevices.size();
    int nbOutputDevices = audioOutputDevices.size();

    inputVolume = inputVolume < 0.0 ? 0.0 : inputVolume > 1.0 ? 1.0 : inputVolume;
    inputIndex = inputIndex < -1 ? -1 : inputIndex > nbInputDevices ? nbInputDevices-1 : inputIndex;
    outputIndex = outputIndex < -1 ? -1 : outputIndex > nbOutputDevices ? nbOutputDevices-1 : outputIndex;

    m_mainWindow.m_audioDeviceInfo.setInputVolume(inputVolume);
    m_mainWindow.m_audioDeviceInfo.setInputDeviceIndex(inputIndex);
    m_mainWindow.m_audioDeviceInfo.setOutputDeviceIndex(outputIndex);

    m_mainWindow.m_dspEngine->setAudioInputVolume(inputVolume);
    m_mainWindow.m_dspEngine->setAudioInputDeviceIndex(inputIndex);
    m_mainWindow.m_dspEngine->setAudioOutputDeviceIndex(outputIndex);

    response.setInputVolume(m_mainWindow.m_audioDeviceInfo.getInputVolume());
    response.setInputIndex(m_mainWindow.m_audioDeviceInfo.getInputDeviceIndex());
    response.setOutputIndex(m_mainWindow.m_audioDeviceInfo.getOutputDeviceIndex());

    return 200;
}

QtMsgType WebAPIAdapterGUI::getMsgTypeFromString(const QString& msgTypeString)
{
    if (msgTypeString == "debug") {
        return QtDebugMsg;
    } else if (msgTypeString == "info") {
        return QtInfoMsg;
    } else if (msgTypeString == "warning") {
        return QtWarningMsg;
    } else if (msgTypeString == "error") {
        return QtCriticalMsg;
    } else {
        return QtDebugMsg;
    }
}

void WebAPIAdapterGUI::getMsgTypeString(const QtMsgType& msgType, QString& levelStr)
{
    switch (msgType)
    {
    case QtDebugMsg:
        levelStr = "debug";
        break;
    case QtInfoMsg:
        levelStr = "info";
        break;
    case QtWarningMsg:
        levelStr = "warning";
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        levelStr = "error";
        break;
    default:
        levelStr = "debug";
        break;
    }
}
