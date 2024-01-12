Readme for DTVKit Configuration
This folder contains the DTVKit.xml file, which serves as the repo xml file for DTVKit.

File Description
The DTVKit.xml file without a suffix corresponds to the primary DTVKit configuration for the current Android version (e.g., T, U).
Files with a suffix represent configurations applicable to specific Android versions (e.g., P, R).
Usage
Select the appropriate DTVKit.xml file based on your Android version.
If you are using the current Android version, choose the file without a suffix.
For other Android versions, select the file with the corresponding suffix and remove the suffix.
By default, we have <include name="google_partner_ref.xml" />.
In case that different ref.xml is needed, please modify "google_partner_ref.xml" to the one needed before use.

Folder Structure
DTVKit.xml: Main configuration file for the current Android version (no suffix).
DTVKit.xml with suffix (e.g., DTVKit_P.xml, DTVKit_R.xml): Configuration files for specific Android versions.
Feel free to contact us for any further assistance or inquiries.

Note:
Ensure that you follow the correct xml file based on your Android version to ensure optimal compatibility with DTVKit.
HBBTV is included by default in T\U xml for the convenience of Coverity code scan. For RD, HBBTV repo can be excluded by removing the corresponding line in xml while building.

For more information or support, please contact zhiwei.jiang.