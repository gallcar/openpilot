#this was initiated by atom(conan)
#partially modified by opkr
from cereal import log
from common.params import Params

from selfdrive.car.hyundai.spdcontroller  import SpdController

import common.log as trace1

LaneChangeState = log.LateralPlan.LaneChangeState


class SpdctrlLong(SpdController):
    def __init__(self, CP=None):
        super().__init__( CP )
        self.params = Params()
        self.cv_Raio = 0.4
        self.cv_Dist = -5
        self.steer_mode = 0
        self.cruise_gap = 0.0
        self.map_enable = False
        self.map_spdlimit_offset = 0
        self.target_speed = 0
        self.target_speed_camera = 0
        self.target_speed_map = 0.0
        self.target_speed_map_counter = 0
        self.target_speed_map_counter1 = 0
        self.target_speed_map_counter2 = 0
        self.map_decel_only = False
        self.map_spdlimit_offset = int(self.params.get("OpkrSpeedLimitOffset", encoding="utf8"))
        self.map_enabled = False
        self.second = 0
        self.curv_hold1 = 0
        self.curv_hold2 = 0
        self.curv_hold3 = 0
        self.curv_hold4 = 0
        self.curv_hold5 = 0

    def update_lead(self, sm, CS, dRel, yRel, vRel):
        self.map_decel_only = CS.out.cruiseState.modeSel == 5
        plan = sm['longitudinalPlan']
        dRele = plan.dRel1 #EON Lead
        yRele = plan.yRel1 #EON Lead
        vRele = plan.vRel1 * 3.6 + 0.5 #EON Lead
        self.second += 1
        if self.second > 100:
            self.map_enabled = self.params.get_bool("OpkrMapEnable")
            self.second = 0
        if self.map_enabled:
            self.target_speed_camera = plan.targetSpeedCamera + round(plan.targetSpeedCamera*0.01*self.map_spdlimit_offset)
        else:
            self.target_speed_camera = CS.out.safetySign + round(CS.out.safetySign*0.01*self.map_spdlimit_offset)
        
        if self.target_speed_camera <= 29:
            self.map_enable = False
            self.target_speed = 0
        elif self.target_speed_camera > 29 and (plan.onSpeedControl if self.map_enabled else CS.on_speed_control):
            self.target_speed = self.target_speed_camera
            self.map_enable = True
        else:
            self.map_enable = False
            self.target_speed = 0

        lead_set_speed = int(round(self.cruise_set_speed_kph))
        lead_wait_cmd = 300

        dRel = 150
        vRel = 0

        #dRel, yRel, vRel = self.get_lead( sm, CS )
        if 1 < dRele < 149:
            dRel = int(dRele)
            vRel = int(vRele)
        elif 1 < CS.lead_distance < 149:
            dRel = int(CS.lead_distance)
            vRel = int(CS.lead_objspd)
        else:
            dRel = 150
            vRel = 0

        dst_lead_distance = int(CS.clu_Vanz*self.cv_Raio)
        
        if dst_lead_distance > 100:
            dst_lead_distance = 100
        #elif dst_lead_distance < 15:
            #dst_lead_distance = 15

        if 1 < dRel < 149:
            d_delta = dRel - dst_lead_distance
            lead_objspd = vRel
        else:
            d_delta = 0
            lead_objspd = 0
 
        if CS.driverAcc_time and not self.map_decel_only:
            if int(CS.VSetDis) < int(round(CS.clu_Vanz)) + 2:
              lead_set_speed = int(round(CS.clu_Vanz)) + 2
              self.seq_step_debug = 0
              lead_wait_cmd = 15
        elif int(round(self.target_speed)) < int(CS.VSetDis) and self.map_enable and ((int(round(self.target_speed)) < int(round(self.cruise_set_speed_kph))) and self.target_speed != 0):
            self.seq_step_debug = 1
            lead_wait_cmd, lead_set_speed = self.get_tm_speed(CS, 10, -1)
        elif CS.CP.resSpeed != 0 and CS.CP.resSpeed < int(CS.VSetDis):
            self.seq_step_debug = 2
            lead_wait_cmd, lead_set_speed = self.get_tm_speed(CS, 8, -1)
        elif CS.out.cruiseState.modeSel in [1,2,4] and 6 < dRel < 149 and lead_objspd < -23 and not self.map_decel_only:
            self.seq_step_debug = 3
            lead_wait_cmd, lead_set_speed = self.get_tm_speed(CS, max(10, dRel-25), -10)
        elif CS.out.cruiseState.modeSel in [1,2,4] and self.cruise_set_speed_kph > int(round((CS.clu_Vanz))) and not self.map_decel_only:
            if 10 > dRel > 3 and lead_objspd <= 0 and 1 < int(CS.clu_Vanz) <= 7 and CS.VSetDis < 45 and ((int(round(self.target_speed)) > int(CS.VSetDis) and self.target_speed != 0) or self.target_speed == 0):
                self.seq_step_debug = 4
                lead_wait_cmd, lead_set_speed = self.get_tm_speed( CS, 8, 5)
            elif 20 > dRel > 3 and lead_objspd > 5 and CS.clu_Vanz <= 25 and CS.VSetDis < 55 and ((int(round(self.target_speed)) > int(CS.VSetDis) and self.target_speed != 0) or self.target_speed == 0):
                self.seq_step_debug = 5
                lead_wait_cmd, lead_set_speed = self.get_tm_speed( CS, 50, 1)
            elif lead_objspd > 0 and int(CS.clu_Vanz)+lead_objspd >= int(CS.VSetDis) and int(CS.clu_Vanz*0.35) < dRel < 149 and ((int(round(self.target_speed)) > int(CS.VSetDis) and self.target_speed != 0) or self.target_speed == 0):
                self.seq_step_debug = 6
                lead_wait_cmd, lead_set_speed = self.get_tm_speed( CS, 20, 1)
            elif CS.clu_Vanz > 80 and lead_objspd < -4 and int(CS.clu_Vanz) <= int(CS.VSetDis) and int(CS.clu_Vanz) >= dRel*1.7 and 1 < dRel < 149:
                self.seq_step_debug = 7
                lead_wait_cmd, lead_set_speed = self.get_tm_speed( CS, max(20, 70+(lead_objspd*2)), -1)
            elif CS.clu_Vanz > 65 and lead_objspd < -4 and int(CS.clu_Vanz) <= int(CS.VSetDis) and int(CS.clu_Vanz) >= dRel*1.9 and 1 < dRel < 149:
                self.seq_step_debug = 8
                lead_wait_cmd, lead_set_speed = self.get_tm_speed( CS, max(20, 80+(lead_objspd*2)), -1)
            # elif CS.clu_Vanz > 40 and lead_objspd < -3 and int(CS.clu_Vanz) <= int(CS.VSetDis) and int(CS.clu_Vanz) >= dRel*2.2 and 1 < dRel < 149:
            #     self.seq_step_debug = 9
            #     lead_wait_cmd, lead_set_speed = self.get_tm_speed( CS, max(20, 80+(lead_objspd*2)), -1)
            elif 65 > CS.clu_Vanz > 30 and lead_objspd < -3 and int(CS.clu_Vanz) <= int(CS.VSetDis) and int(CS.clu_Vanz) >= dRel*0.85 and 1 < dRel < 149:
                self.seq_step_debug = 10
                lead_wait_cmd, lead_set_speed = self.get_tm_speed( CS, max(15, 230-(abs(lead_objspd**3))), -1)
            elif 65 > CS.clu_Vanz > 30 and lead_objspd <= 0 and int(CS.clu_Vanz)+3 < int(CS.VSetDis) and int(CS.clu_Vanz) >= dRel*0.85 and 1 < dRel < 149:
                self.seq_step_debug = 11
                lead_wait_cmd, lead_set_speed = self.get_tm_speed( CS, 15, -1)
            elif 7 < int(CS.clu_Vanz) < 30 and lead_objspd < 0 and CS.VSetDis > 30:
                self.seq_step_debug = 12
                lead_wait_cmd, lead_set_speed = self.get_tm_speed( CS, 15, -5)
            elif lead_objspd <= 0 and int(CS.clu_Vanz)+5 <= int(CS.VSetDis) and int(CS.clu_Vanz) > 40 and 1 < dRel < 149:
                self.seq_step_debug = 13
                lead_wait_cmd, lead_set_speed = self.get_tm_speed( CS, 15, -1)
            elif d_delta == 0 and lead_objspd == 0 and self.cruise_set_speed_kph > int(CS.VSetDis) and dRel > 149 and ((int(round(self.target_speed)) > int(CS.VSetDis) and self.target_speed != 0) or self.target_speed == 0):
                self.seq_step_debug = 14
                lead_wait_cmd, lead_set_speed = self.get_tm_speed( CS, 50, 1)
            elif lead_objspd == 0 and int(CS.clu_Vanz) == 0 and dRel <= 6:
                self.seq_step_debug = 15
            else:
                self.seq_step_debug = 16
        elif CS.out.cruiseState.modeSel in [1,2,4] and lead_objspd >= 0 and CS.clu_Vanz >= int(CS.VSetDis) and int(CS.clu_Vanz * 0.5) < dRel < 149 and not self.map_decel_only:
            self.seq_step_debug = 17
        elif CS.out.cruiseState.modeSel in [1,2,4] and lead_objspd < 0 and int(CS.clu_Vanz * 0.5) >= dRel > 1 and not self.map_decel_only:
            self.seq_step_debug = 18
            lead_wait_cmd, lead_set_speed = self.get_tm_speed( CS, 50, -1)
        elif (self.map_decel_only or CS.out.cruiseState.modeSel == 3) and self.cruise_set_speed_kph > int(round(CS.VSetDis)) and ((int(round(self.target_speed)) > int(CS.VSetDis) and self.target_speed != 0) or self.target_speed == 0):
            self.seq_step_debug = 19
            lead_wait_cmd, lead_set_speed = self.get_tm_speed( CS, 30, 1)
        else:
            self.seq_step_debug = 20

        return lead_wait_cmd, lead_set_speed

    def update_curv(self, CS, sm, curve_speed):
        wait_time_cmd = 0
        set_speed = self.cruise_set_speed_kph

        #if self.cruise_set_speed_kph >= 100:
        if CS.out.cruiseState.modeSel in [1,3,4] and sm['lateralPlan'].laneChangeState == LaneChangeState.off and not (CS.out.leftBlinker or CS.out.rightBlinker)and not self.map_decel_only:
            cam_speed = self.target_speed if self.target_speed > 0 else 255
            if curve_speed <= 35+self.curv_hold5 and CS.clu_Vanz > 40 and CS.lead_distance >= 15:
                set_speed = min(cam_speed, 40, self.cruise_set_speed_kph, self.cruise_set_speed_kph - int(CS.clu_Vanz * 0.2))
                self.seq_step_debug = 35
                wait_time_cmd = 15
                self.curv_hold5 = 9
            elif curve_speed < 45+self.curv_hold4 and CS.clu_Vanz > 40 and CS.lead_distance >= 15:
                set_speed = min(cam_speed, 45, self.cruise_set_speed_kph, self.cruise_set_speed_kph - int(CS.clu_Vanz * 0.15))
                self.seq_step_debug = 34
                wait_time_cmd = 30
                self.curv_hold4 = 10
            elif curve_speed < 60+self.curv_hold3 and CS.clu_Vanz > 40 and CS.lead_distance >= 15:
                set_speed = min(cam_speed, 60, self.cruise_set_speed_kph, self.cruise_set_speed_kph - int(CS.clu_Vanz * 0.1))
                self.seq_step_debug = 33
                wait_time_cmd = 45
                self.curv_hold3 = 10
            elif curve_speed < 75+self.curv_hold2 and CS.clu_Vanz > 40 and CS.lead_distance >= 15:
                set_speed = min(cam_speed, 75, self.cruise_set_speed_kph, self.cruise_set_speed_kph - int(CS.clu_Vanz * 0.075))
                self.seq_step_debug = 32
                wait_time_cmd = 60
                self.curv_hold2 = 10
            elif curve_speed < 90+self.curv_hold1 and CS.clu_Vanz > 40 and CS.lead_distance >= 15:
                set_speed = min(cam_speed, 90, self.cruise_set_speed_kph, self.cruise_set_speed_kph - int(CS.clu_Vanz * 0.05))
                self.seq_step_debug = 31
                wait_time_cmd = 75
                self.curv_hold1 = 10
            else:
                self.curv_hold1 = 0
                self.curv_hold2 = 0
                self.curv_hold3 = 0
                self.curv_hold4 = 0
                self.curv_hold5 = 0

        return wait_time_cmd, set_speed


    def update_log(self, CS, set_speed, target_set_speed, long_wait_cmd ):
        str3 = 'MODE={}  BS={:1.0f}/{:1.0f}  VL={:03.0f}/{:03.0f}  TM={:03.0f}/{:03.0f}  TS={:03.0f}'.format(CS.out.cruiseState.modeSel, CS.CP.mdpsBus, CS.CP.sccBus, set_speed, CS.VSetDis, long_wait_cmd, self.long_curv_timer, int(round(self.target_speed)) )
        str4 = '  RD=D:{:03.0f}/V:{:03.0f}  CG={:1.0f}  DG={}'.format(  CS.lead_distance, CS.lead_objspd, CS.cruiseGapSet, self.seq_step_debug )

        str5 = str3 + str4
        trace1.printf2( str5 )